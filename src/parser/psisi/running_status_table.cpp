#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "running_status_table.h"
#include "utils.h"

namespace parser {



running_status_table::running_status_table (void)
{
	tables_.clear();
}

running_status_table::running_status_table (int fifo_num)
	:section_parser (fifo_num)
{
	tables_.clear();
}

running_status_table::~running_status_table (void)
{
	clear();
}

void running_status_table::on_section_completed (const section_info *p_comp_section)
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

bool running_status_table::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();

	int status_len = (int) p_table->header.section_length ;
	if (status_len <= RST_STATUS_FIX_LEN) {
		_UTL_LOG_W ("invalid RST");
		return false;
	}

	while (status_len > 0) {

		table::status stt ;

		stt.transport_stream_id = *p << 8 | *(p+1);
		stt.original_network_id = *(p+2) << 8 | *(p+3);
		stt.service_id = *(p+4) << 8 | *(p+5);
		stt.event_id = *(p+6) << 8 | *(p+7);
		stt.running_status = *(p+8) & 0x07;

		p += RST_STATUS_FIX_LEN;

		status_len -= RST_STATUS_FIX_LEN ;
		if (status_len < 0) {
			_UTL_LOG_W ("invalid RST status");
			return false;
		}

		p_table->statuses.push_back (stt);
	}

	return true;
}

void running_status_table::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	tables_.push_back (p_table);
}

void running_status_table::release_tables (void)
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

void running_status_table::dump_tables (void)
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

void running_status_table::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	
	}
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	std::vector<table::status>::const_iterator iter_stt = p_table->statuses.begin();
	for (; iter_stt != p_table->statuses.end(); ++ iter_stt) {
		_UTL_LOG_I ("\n--  statuses  --\n");
		_UTL_LOG_I ("transport_stream_id [0x%04x]\n", iter_stt->transport_stream_id);
		_UTL_LOG_I ("original_network_id [0x%04x]\n", iter_stt->original_network_id);
		_UTL_LOG_I ("service_id          [0x%04x]\n", iter_stt->service_id);
		_UTL_LOG_I ("event_id            [0x%04x]\n", iter_stt->event_id);
		_UTL_LOG_I ("running_status      [0x%01x]\n", iter_stt->running_status);
	}

	_UTL_LOG_I ("\n");
}

void running_status_table::clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}


} // namespace parser
