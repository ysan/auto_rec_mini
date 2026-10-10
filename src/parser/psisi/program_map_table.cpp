#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "program_map_table.h"
#include "utils.h"

namespace parser {



program_map_table::program_map_table (void)
{
	tables_.clear();
}

program_map_table::program_map_table (int fifo_num)
	:section_parser (fifo_num)
{
	tables_.clear();
}

program_map_table::~program_map_table (void)
{
	clear();
}


void program_map_table::on_section_completed (const section_info *p_comp_section)
{
	if (!p_comp_section) {
		return;
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

bool program_map_table::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();
	p_table->reserved_3 = (*p >> 5) & 0x07;
	p_table->PCR_PID = (*p & 0x1f) << 8 | *(p+1);
	p_table->reserved_4 = (*(p+2) >> 4) & 0x0f;
	p_table->program_info_length = (*(p+2) & 0x0f) << 8 | *(p+3);

	p += PMT_FIX_LEN;

	int n = (int)p_table->program_info_length;
	while (n > 0) {
		descriptor desc (p);
		if (!desc.is_valid) {
			_UTL_LOG_W ("invalid PMT desc 1");
			_UTL_LOG_W ("program_info_length %d %d", (int)p_table->program_info_length, n);
			_UTL_LOG_W ("tag 0x%02x", *p);
			_UTL_LOG_W ("length %d", *(p+1));
			return false;
		}
		p_table->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	int stream_len = (int) (p_table->header.section_length - p_table->program_info_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - PMT_FIX_LEN);
	if (stream_len <= PMT_STREAM_FIX_LEN) {
		_UTL_LOG_W ("invalid PMT stream");
		return false;
	}

	while (stream_len > 0) {

		table::stream strm ;

		strm.stream_type = *p;
		strm.reserved_1 = (*(p+1) >> 5) & 0x07;
		strm.elementary_PID = (*(p+1) & 0x1f) << 8 | *(p+2);
		strm.reserved_2 = (*(p+3) >> 4) & 0x0f;
		strm.ES_info_length = (*(p+3) & 0x0f) << 8 | *(p+4);

		p += PMT_STREAM_FIX_LEN;

		int n = (int)strm.ES_info_length ;
		while (n > 0) {
			descriptor desc (p);
			if (!desc.is_valid) {
				_UTL_LOG_W ("invalid PMT desc 2");
				return false;
			}
			strm.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		stream_len -= (PMT_STREAM_FIX_LEN + strm.ES_info_length) ;
		if (stream_len < 0) {
			_UTL_LOG_W ("invalid PMT stream");
			return false;
		}

		p_table->streams.push_back (strm);
	}

	return true;
}

void program_map_table::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	tables_.push_back (p_table);
}

void program_map_table::release_tables (void)
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

void program_map_table::dump_tables (void)
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

void program_map_table::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("PID [0x%04x]\n", this->get_pid ());
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("PCR_PID             [0x%04x]\n", p_table->PCR_PID);
	_UTL_LOG_I ("program_info_length [%d]\n", p_table->program_info_length);

	std::vector<descriptor>::const_iterator iter_desc = p_table->descriptors.begin();
	for (; iter_desc != p_table->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		descriptor_common::dump (iter_desc->tag, *iter_desc);
	}

	std::vector<table::stream>::const_iterator iter_strm = p_table->streams.begin();
	for (; iter_strm != p_table->streams.end(); ++ iter_strm) {
		_UTL_LOG_I ("\n--  stream  --\n");
		_UTL_LOG_I ("stream_type    [0x%02x]\n", iter_strm->stream_type);
		_UTL_LOG_I ("elementary_PID [0x%04x]\n", iter_strm->elementary_PID);
		_UTL_LOG_I ("ES_info_length [%d]\n", iter_strm->ES_info_length);

		std::vector<descriptor>::const_iterator iter_desc = iter_strm->descriptors.begin();
		for (; iter_desc != iter_strm->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			descriptor_common::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void program_map_table:: clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}

void program_map_table:: force_clear (void)
{
	release_tables ();

	detach_all_section_list ();
}


} // namespace parser
