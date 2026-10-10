#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "network_information_table.h"
#include "utils.h"

namespace parser {



network_information_table::network_information_table (void)
{
	tables_.clear();
}

network_information_table::network_information_table (int fifo_num)
	:section_parser (fifo_num)
{
	tables_.clear();
}

network_information_table::~network_information_table (void)
{
	clear();
}

void network_information_table::on_section_completed (const section_info *p_comp_section)
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

bool network_information_table::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();
	p_table->reserved_future_use_2 = (*p >> 4) & 0x0f;
	p_table->network_descriptors_length = (*p & 0x0f) << 8 | *(p+1);

	p += NIT_FIX_LEN;

	int n = (int)p_table->network_descriptors_length;
	while (n > 0) {
		descriptor desc (p);
		if (!desc.is_valid) {
			_UTL_LOG_W ("invalid NIT desc");
			return false;
		}
		p_table->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	p_table->reserved_future_use_3 = (*p >> 4) & 0x0f;
	p_table->transport_stream_loop_length = (*p & 0x0f) << 8 | *(p+1);

	p += NIT_FIX2_LEN;

	int stream_len = (int) (p_table->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - NIT_FIX_LEN - p_table->network_descriptors_length - NIT_FIX2_LEN);
	if (stream_len <= NIT_STREAM_FIX_LEN) {
		_UTL_LOG_W ("invalid NIT stream");
		return false;
	}

	if (stream_len != p_table->transport_stream_loop_length) {
		_UTL_LOG_W ("invalid NIT stream");
		return false;
	}

	while (stream_len > 0) {

		table::stream strm ;

		strm.transport_stream_id = *p << 8 | *(p+1);
		strm.original_network_id = (*(p+2) << 8) | *(p+3);
		strm.reserved_future_use = (*(p+4) >> 4) & 0x0f;
		strm.transport_descriptors_length = (*(p+4) & 0x0f) << 8 | *(p+5);

		p += NIT_STREAM_FIX_LEN;

		int n = (int)strm.transport_descriptors_length;
		while (n > 0) {
			descriptor desc (p);
			if (!desc.is_valid) {
				_UTL_LOG_W ("invalid NIT desc2");
				return false;
			}
			strm.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		stream_len -= (NIT_STREAM_FIX_LEN + strm.transport_descriptors_length) ;
		if (stream_len < 0) {
			_UTL_LOG_W ("invalid NIT stream");
			return false;
		}

		p_table->streams.push_back (strm);
	}

	return true;
}

void network_information_table::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	tables_.push_back (p_table);
}

void network_information_table::release_tables (void)
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

void network_information_table::dump_tables (void)
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

void network_information_table::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	}
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                         [0x%02x]\n", p_table->header.table_id);
	_UTL_LOG_I ("network_id                       [0x%04x]\n", p_table->header.table_id_extension);
	_UTL_LOG_I ("network_descriptors_length       [%d]\n", p_table->network_descriptors_length);

	std::vector<descriptor>::const_iterator iter_desc = p_table->descriptors.begin();
	for (; iter_desc != p_table->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		descriptor_common::dump (iter_desc->tag, *iter_desc);
	}

	_UTL_LOG_I ("\ntransport_stream_loop_length     [%d]\n", p_table->transport_stream_loop_length);

	std::vector<table::stream>::const_iterator iter_strm = p_table->streams.begin();
	for (; iter_strm != p_table->streams.end(); ++ iter_strm) {
		_UTL_LOG_I ("\n--  stream  --\n");
		_UTL_LOG_I ("transport_stream_id          [0x%04x]\n", iter_strm->transport_stream_id);
		_UTL_LOG_I ("original_network_id          [0x%04x]\n", iter_strm->original_network_id);
		_UTL_LOG_I ("transport_descriptors_length [%d]\n", iter_strm->transport_descriptors_length);

		std::vector<descriptor>::const_iterator iter_desc = iter_strm->descriptors.begin();
		for (; iter_desc != iter_strm->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			descriptor_common::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void network_information_table::clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}


} // namespace parser
