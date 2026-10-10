#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "broadcaster_information_table.h"
#include "utils.h"

namespace parser {



broadcaster_information_table::broadcaster_information_table (void)
{
	tables_.clear();
}

broadcaster_information_table::broadcaster_information_table (int fifo_num)
	:section_parser (fifo_num)
{
	tables_.clear();
}

broadcaster_information_table::~broadcaster_information_table (void)
{
	clear();
}

void broadcaster_information_table::on_section_completed (const section_info *p_comp_section)
{
	if (!p_comp_section) {
		return ;
	}

	table *p_table = new table ();
	if (!parse (p_comp_section, p_table)) {
		delete p_table;
		p_table = NULL;
		detach_section_list (p_comp_section);
		return ;
	}

	append_table (p_table);

	// debug dump
	if (arec::common::utils::get_logger()->get_log_level() <= arec::common::logger::level::debug) {
		dump_table (p_table);
	}
}

bool broadcaster_information_table::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();
	p_table->reserved_future_use_2 = (*p >> 5) & 0x07;
	p_table->broadcast_view_propriety = (*p >> 4) & 0x01;
	p_table->first_descriptors_length = (*p & 0x0f) << 8 | *(p+1);

	p += BIT_FIX_LEN;

	int n = (int)p_table->first_descriptors_length;
	while (n > 0) {
		descriptor desc (p);
		if (!desc.is_valid) {
			_UTL_LOG_W ("invalid BIT desc");
			return false;
		}
		p_table->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	int barodcaster_len = (int) (p_table->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - BIT_FIX_LEN - p_table->first_descriptors_length);
	if (barodcaster_len <= BIT_BROADCASTER_FIX_LEN) {
		_UTL_LOG_W ("invalid BIT broadcaster");
		return false;
	}

	while (barodcaster_len > 0) {

		table::broadcaster brd;

		brd.broadcaster_id = *p;
		brd.reserved_future_use = (*(p+1) >> 4) & 0x0f;
		brd.broadcaster_descriptors_length = (*(p+1) & 0xf) << 8 | *(p+2);

		p += BIT_BROADCASTER_FIX_LEN;

		int n = (int)brd.broadcaster_descriptors_length;
		while (n > 0) {
			descriptor desc (p);
			if (!desc.is_valid) {
				_UTL_LOG_W ("invalid BIT desc2");
				return false;
			}
			brd.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		barodcaster_len -= (BIT_BROADCASTER_FIX_LEN + brd.broadcaster_descriptors_length) ;
		if (barodcaster_len < 0) {
			_UTL_LOG_W ("invalid BIT broadcaster");
			return false;
		}

		p_table->broadcasters.push_back (brd);
	}

	return true;
}

void broadcaster_information_table::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	tables_.push_back (p_table);
}

void broadcaster_information_table::release_tables (void)
{
	if (tables_.size() == 0) {
		return;
	}

	std::vector<table*>::iterator iter = tables_.begin(); 
	for (; iter != tables_.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	tables_.clear();
}

void broadcaster_information_table::dump_tables (void)
{
	if (tables_.size() == 0) {
		return;
	}

	std::vector<table*>::const_iterator iter = tables_.begin(); 
	for (; iter != tables_.end(); ++ iter) {
		table *p_table = *iter;
		dump_table (p_table);
	}
}

void broadcaster_information_table::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                           [0x%02x]\n", p_table->header.table_id);
	_UTL_LOG_I ("original_network_id                [0x%04x]\n", p_table->header.table_id_extension);
	_UTL_LOG_I ("broadcast_view_propriety           [0x%02x]\n", p_table->broadcast_view_propriety);
	_UTL_LOG_I ("first_descriptors_length           [%d]\n", p_table->first_descriptors_length);

	std::vector<descriptor>::const_iterator iter_desc = p_table->descriptors.begin();
	for (; iter_desc != p_table->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		descriptor_common::dump (iter_desc->tag, *iter_desc);
	}

	std::vector<table::broadcaster>::const_iterator iter_brd = p_table->broadcasters.begin();
	for (; iter_brd != p_table->broadcasters.end(); ++ iter_brd) {
		_UTL_LOG_I ("\n--  broadcaster  --\n");
		_UTL_LOG_I ("broadcaster_id                 [0x%02x]\n", iter_brd->broadcaster_id);
		_UTL_LOG_I ("broadcaster_descriptors_length [%d]\n", iter_brd->broadcaster_descriptors_length);

		std::vector<descriptor>::const_iterator iter_desc = iter_brd->descriptors.begin();
		for (; iter_desc != iter_brd->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			descriptor_common::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void broadcaster_information_table::clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}


} // namespace parser
