#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "program_association_table.h"
#include "utils.h"

namespace parser {




program_association_table::program_association_table (void)
{
	tables_.clear();
}

program_association_table::program_association_table (int fifo_num)
	:section_parser (fifo_num)
{
	tables_.clear();
}

program_association_table::~program_association_table (void)
{
	clear();
}

void program_association_table::on_section_completed (const section_info *p_comp_section)
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

	refresh_sub_tables (p_table) ;

	append_table (p_table);

	// debug dump
	if (arec::common::utils::get_logger()->get_log_level() <= arec::common::logger::level::debug) {
		dump_table (p_table);
	}

}

bool program_association_table::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();

	int data_part_len = (int)p_comp_section->get_data_part_len ();
	if (data_part_len < 4) {
		_UTL_LOG_E ("invalid PAT  data_part_len=[%d]\n", data_part_len);
		return false;
	}

	if ((data_part_len % 4) != 0) {
		// 1ループの大きさは4の倍数
		_UTL_LOG_E ("invalid PAT (not multiples of 4)\n");
		return false;
	}

	while (data_part_len > 0) {

		table::program prog ;

		prog.program_number = ((*p << 8) | *(p+1)) & 0xffff;
		prog.program_map_PID = (((*(p+2) & 0x01f) << 8) | *(p+3)) & 0xffff;

		p += 4;

		data_part_len -= 4 ;
		if (data_part_len < 0) {
			_UTL_LOG_W ("invalid PAT program");
			return false;
		}

		p_table->programs.push_back (prog);
	}

	return true;
}

void program_association_table::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	tables_.push_back (p_table);
}

void program_association_table::release_tables (void)
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

void program_association_table::release_table (table *p_erase)
{
	if (!p_erase) {
		return;
	}

	if (tables_.size() == 0) {
		return;
	}

	std::vector<table*>::iterator iter = tables_.begin(); 
	for (; iter != tables_.end(); ++ iter) {
		if (*iter == p_erase) {
			delete (*iter);
			(*iter) = NULL;
			iter = tables_.erase(iter);
			break;
		}
	}
}

void program_association_table::dump_tables (void)
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

void program_association_table::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	std::vector<table::program>::const_iterator iter_prog = p_table->programs.begin();
	for (; iter_prog != p_table->programs.end(); ++ iter_prog) {
		_UTL_LOG_I ("\n--  program  --\n");
		_UTL_LOG_I ("program_number  [0x%04x]\n", iter_prog->program_number);
		if (iter_prog->program_number == 0) {
			_UTL_LOG_I ("network_PID     [0x%04x]\n", iter_prog->program_map_PID);
		} else {
			_UTL_LOG_I ("program_map_PID [0x%04x]\n", iter_prog->program_map_PID);
		}
	}

	_UTL_LOG_I ("\n");
}

void program_association_table::clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}

bool program_association_table::refresh_sub_tables_by_version_number (table* p_new_table)
{
	if (!p_new_table) {
		return false;
	}


	// sub-table identify
	uint8_t new_tblid = p_new_table->header.table_id;
	uint16_t new_tsid = p_new_table->header.table_id_extension;
	uint8_t new_ver = p_new_table->header.version_number;
	uint8_t new_sec_num = p_new_table->header.section_number;

	table *p_erase = NULL;

	std::vector<table*>::const_iterator iter = tables_.begin();
    for (; iter != tables_.end(); ++ iter) {
		table *p_table = *iter;

		// check every sub-table
		if (
			(new_tblid == p_table->header.table_id) &&
			(new_tsid == p_table->header.table_id_extension) &&
			(new_sec_num == p_table->header.section_number)
		) {
			// PAT も現在送出中のものを「最新受信」として採用する。
			// version_number は 5bit 循環 (0..31) で、版が若返る (減少する) ことが
			// あり、単純な大小比較では新旧を判定できない。
			// そのため版が異なれば新しい受信として差し替え、同一版のみ重複として扱う。
			if (new_ver != p_table->header.version_number) {

				// version changed -> replace with the latest received
				p_erase = p_table;
				break;

			} else {
				// duplicate sections should be checked by section_parser
				_UTL_LOG_E (
					"BUG: same version.  new_ver:[0x%02x]  p_table->header.version_number:[0x%02x]",
					new_ver,
					p_table->header.version_number
				);
			}
		}
	}

	if (p_erase) {

        _UTL_LOG_I (
            "PAT new ver >> tblid:[0x%02x] tsid:[0x%04x] num:[0x%02x] ver:[0x%02x]->[0x%02x]",
            new_tblid,
            new_tsid,
            new_sec_num,
            p_erase->header.version_number,
            new_ver
        );

		release_table (p_erase);
		return true;
	} else {
		return false;
	}
}

void program_association_table::refresh_sub_tables (table* p_new_table)
{
	if (!p_new_table) {
		return ;
	}

	while (1) {
		if (!refresh_sub_tables_by_version_number (p_new_table)) {
			break;
		}
	}
}


} // namespace parser
