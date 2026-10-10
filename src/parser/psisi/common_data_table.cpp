#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <memory>

#include "common_data_table.h"
#include "utils.h"

namespace parser {



common_data_table::common_data_table (void)
{
	tables_.clear();
}

common_data_table::common_data_table (int fifo_num)
	:section_parser (fifo_num)
{
	tables_.clear();
}

common_data_table::common_data_table (size_t pool_size)
	:section_parser (pool_size)
{
	tables_.clear();
}

common_data_table::~common_data_table (void)
{
	clear();
}

void common_data_table::on_section_completed (const section_info *p_comp_section)
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

bool common_data_table::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();
	p_table->original_network_id = *p << 8 | *(p+1);
	p_table->data_type = *(p+2);
	p_table->reserved_future_use = (*(p+3) & 0xf0) >> 4;
	p_table->descriptors_loop_length = (*(p+3) & 0x0f) << 8 | *(p+4);

	p += CDT_FIX_LEN;
	int n = (int)p_table->descriptors_loop_length;
	while (n > 0) {
		descriptor desc (p);
		if (!desc.is_valid) {
			_UTL_LOG_W ("invalid CDT desc");
			return false;
		}
		p_table->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	if (p_table->data_type != 0x01) {
		return true;
	}

	int data_len = (int) (p_table->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - CDT_FIX_LEN - p_table->descriptors_loop_length);
	if (data_len <= CDT_DATA_FIX_LEN) {
		_UTL_LOG_W ("invalid CDT data");
		return false;
	}

	p_table->data.logo_type = *p;
	p_table->data.reserved_future_use_1 = (*(p+1) & 0xfe) >> 1;
	p_table->data.logo_id = (*(p+1) & 0x01) << 8 | *(p+2);
	p_table->data.reserved_future_use_2 = (*(p+3) & 0xf0) >> 4;
	p_table->data.logo_version = (*(p+3) & 0x0f) << 8 | *(p+4);
	p_table->data.data_size =  *(p+5) << 8 | *(p+6);

	if (data_len != (CDT_DATA_FIX_LEN + p_table->data.data_size)) {
		_UTL_LOG_W ("invalid CDT data_byte");
		return false;
	}

	p += CDT_DATA_FIX_LEN;
	std::unique_ptr<uint8_t[]> up (new uint8_t[p_table->data.data_size]);
	memcpy (up.get(), p, p_table->data.data_size);
	p_table->data.data_byte.swap(up);

	return true;
}

void common_data_table::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	tables_.push_back (p_table);
}

void common_data_table::release_tables (void)
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

void common_data_table::dump_tables (void)
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

void common_data_table::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                  [0x%02x]\n", p_table->header.table_id);
	_UTL_LOG_I ("download_data_id          [0x%04x]\n", p_table->header.table_id_extension);
	_UTL_LOG_I ("original_network_id       [0x%04x]\n", p_table->original_network_id);
	_UTL_LOG_I ("data_type                 [0x%02x]\n", p_table->data_type);
	_UTL_LOG_I ("descriptor_loop_length    [0x%04x]\n", p_table->descriptors_loop_length);

	std::vector<descriptor>::const_iterator iter_desc = p_table->descriptors.begin();
	for (; iter_desc != p_table->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		descriptor_common::dump (iter_desc->tag, *iter_desc);
	}

	if (p_table->data_type == 0x01) {
		_UTL_LOG_I ("\n--  service  --\n");
		_UTL_LOG_I ("logo_type       [0x%02x]\n", p_table->data.logo_type);
		_UTL_LOG_I ("logo_id         [0x%04x]\n", p_table->data.logo_id);
		_UTL_LOG_I ("logo_version    [0x%04x]\n", p_table->data.logo_version);
		_UTL_LOG_I ("data_size       [0x%04x]\n", p_table->data.data_size);
		_UTL_LOG_I ("data_byte\n");
		arec::common::utils::dumper(p_table->data.data_byte.get(), p_table->data.data_size);
	}

	_UTL_LOG_I ("\n");
}

void common_data_table::clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}


} // namespace parser
