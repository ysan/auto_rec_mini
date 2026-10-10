#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "conditional_access_table.h"
#include "utils.h"

namespace parser {



conditional_access_table::conditional_access_table (void)
{
	tables_.clear();
}

conditional_access_table::conditional_access_table (int fifo_num)
	:section_parser (fifo_num)
{
	tables_.clear();
}

conditional_access_table::~conditional_access_table (void)
{
	clear();
}

void conditional_access_table::on_section_completed (const section_info *p_comp_section)
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

bool conditional_access_table::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();

	int n = (int) (p_table->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN);

	while (n > 0) {
		descriptor desc (p);
		if (!desc.is_valid) {
			_UTL_LOG_W ("invalid CAT desc");
			return false;
		}
		p_table->descriptors.push_back (desc);
		n -= (2 + *(p + 1));
		p += (2 + *(p + 1));
	}

	return true;
}

void conditional_access_table::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	tables_.push_back (p_table);
}

void conditional_access_table::release_tables (void)
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

void conditional_access_table::dump_tables (void)
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

void conditional_access_table::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	}
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                         [0x%02x]\n", p_table->header.table_id);

	std::vector<descriptor>::const_iterator iter_desc = p_table->descriptors.begin();
	for (; iter_desc != p_table->descriptors.end(); ++ iter_desc) {
		_UTL_LOG_I ("\n--  descriptor  --\n");
		descriptor_common::dump (iter_desc->tag, *iter_desc);
	}

	_UTL_LOG_I ("\n");
}

void conditional_access_table::clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}


} // namespace parser
