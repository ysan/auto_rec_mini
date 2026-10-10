#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "service_description_table.h"
#include "utils.h"

namespace parser {



service_description_table::service_description_table (void)
{
	tables_.clear();
}

service_description_table::service_description_table (int fifo_num)
	:section_parser (fifo_num)
{
	tables_.clear();
}

service_description_table::~service_description_table (void)
{
	clear();
}

void service_description_table::on_section_completed (const section_info *p_comp_section)
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

bool service_description_table::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();
	p_table->original_network_id = *p << 8 | *(p+1);
	p_table->reserved_future_use_2 = *(p+2);

	p += SDT_FIX_LEN;

	int service_len = (int) (p_table->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - SDT_FIX_LEN);
	if (service_len <= SDT_SERVICE_FIX_LEN) {
		_UTL_LOG_W ("invalid SDT service");
		return false;
	}

	while (service_len > 0) {

		table::service svc ;

		svc.service_id = *p << 8 | *(p+1);
		svc.reserved_future_use = *(p+2) & 0x07;
		svc.EIT_user_defined_flags = (*(p+2) >> 2) & 0x07;
		svc.EIT_schedule_flag = (*(p+2) >> 1) & 0x01;
		svc.EIT_present_following_flag = *(p+2) & 0x01;
		svc.running_status = (*(p+3) >> 5) & 0x07;
		svc.free_CA_mode = (*(p+3) >> 4) & 0x01;
		svc.descriptors_loop_length = (*(p+3) & 0x0f) << 8 | *(p+4);

		p += SDT_SERVICE_FIX_LEN;
		int n = (int)svc.descriptors_loop_length;
		while (n > 0) {
			descriptor desc (p);
			if (!desc.is_valid) {
				_UTL_LOG_W ("invalid SDT desc");
				return false;
			}
			svc.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		service_len -= (SDT_SERVICE_FIX_LEN + svc.descriptors_loop_length) ;
		if (service_len < 0) {
			_UTL_LOG_W ("invalid SDT stream");
			return false;
		}

		p_table->services.push_back (svc);
	}

	return true;
}

void service_description_table::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	tables_.push_back (p_table);
}

void service_description_table::release_tables (void)
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

void service_description_table::dump_tables (void)
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

void service_description_table::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                  [0x%02x]\n", p_table->header.table_id);
	_UTL_LOG_I ("transport_stream_id       [0x%04x]\n", p_table->header.table_id_extension);
	_UTL_LOG_I ("original_network_id       [0x%04x]\n", p_table->original_network_id);

	std::vector<table::service>::const_iterator iter_svc = p_table->services.begin();
	for (; iter_svc != p_table->services.end(); ++ iter_svc) {
		_UTL_LOG_I ("\n--  service  --\n");
		_UTL_LOG_I ("service_id                 [0x%04x]\n", iter_svc->service_id);
		_UTL_LOG_I ("EIT_user_defined_flags     [0x%01x]\n", iter_svc->EIT_user_defined_flags);
		_UTL_LOG_I ("EIT_schedule_flag          [0x%01x]\n", iter_svc->EIT_schedule_flag);
		_UTL_LOG_I ("EIT_present_following_flag [0x%01x]\n", iter_svc->EIT_present_following_flag);
		_UTL_LOG_I ("running_status             [0x%01x]\n", iter_svc->running_status);
		_UTL_LOG_I ("free_CA_mode               [0x%01x]\n", iter_svc->free_CA_mode);
		_UTL_LOG_I ("descriptors_loop_length    [%d]\n", iter_svc->descriptors_loop_length);

		std::vector<descriptor>::const_iterator iter_desc = iter_svc->descriptors.begin();
		for (; iter_desc != iter_svc->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			descriptor_common::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void service_description_table::clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}


} // namespace parser
