#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <algorithm>

#include "event_information_table_sched.h"
#include "utils.h"

namespace parser {



//bool _comp__service_id (const event_information_table_sched::table* l, const event_information_table_sched::table* r) {
//	return r->header.table_id_extension > l->header.table_id_extension;
//}

//bool _comp__table_id (const event_information_table_sched::table* l, const event_information_table_sched::table* r) {
//	if (r->header.table_id_extension == l->header.table_id_extension) {
//		if (r->header.table_id > l->header.table_id) {
//			return true;
//		}
//	}
//
//	return false;
//}

//bool _comp__section_number (const event_information_table_sched::table* l, const event_information_table_sched::table* r) {
//	if (r->header.table_id_extension == l->header.table_id_extension) {
//		if (r->header.table_id == l->header.table_id) {
//			if (r->header.section_number > l->header.section_number) {
//				return true;
//			}
//		}
//	}
//
//	return false;
//}


event_information_table_sched::event_information_table_sched (size_t pool_size)
	:section_parser (pool_size)
{
	tables_.clear();
}

event_information_table_sched::event_information_table_sched (size_t pool_size, int fifo_num)
	:section_parser (pool_size, fifo_num)
{
	tables_.clear();
}

event_information_table_sched::event_information_table_sched (size_t pool_size, int fifo_num, IEventScheduleHandler *p_hander)
	:section_parser (pool_size, fifo_num)
{
	tables_.clear();

	if (p_hander) {
		event_schedule_handler_ = p_hander;
	}
}

event_information_table_sched::~event_information_table_sched (void)
{
	clear();
}


bool event_information_table_sched::on_section_started (const section_info *p_section)
{
	if (!p_section) {
		return false;
	}


	// pf or schedule ?
	uint8_t _tbl_id = p_section->get_header ()->table_id;
	if (_tbl_id == TBLID_EIT_PF_A || _tbl_id == TBLID_EIT_PF_O) {
		// p/f  --> ignore
		return false;

	} else if (
		(_tbl_id >= TBLID_EIT_SCHED_A && _tbl_id <= TBLID_EIT_SCHED_A + 0xf) ||
		(_tbl_id >= TBLID_EIT_SCHED_O && _tbl_id <= TBLID_EIT_SCHED_O + 0xf)
	) {
		// schedule
		return true;

	} else {
		_UTL_LOG_W ("EIT unknown table_id [0x%02x]", _tbl_id);
		return false;
	}
}

void event_information_table_sched::on_section_completed (const section_info *p_comp_section)
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


	if (is_duplicate_sub_tables (p_table)) {
		delete p_table;
		p_table = NULL;
		detach_section_list (p_comp_section);
		return ;
	}


	refresh_sub_tables (p_table) ;

	append_table (p_table);


// need mutex
//	std::stable_sort (tables_.begin(), tables_.end(), _comp__service_id);
//	std::stable_sort (tables_.begin(), tables_.end(), _comp__table_id);
//	std::stable_sort (tables_.begin(), tables_.end(), _comp__section_number);


	// debug dump
	if (arec::common::utils::get_logger()->get_log_level() <= arec::common::logger::level::debug) {
		dumpTables_simple ();
//TODO mutex
		std::lock_guard<std::recursive_mutex> lock (mutex_tables_);
		dump_table (p_table);
	}


	// EIT schedule is not create section-list
	detach_section_list (p_comp_section);


	if (event_schedule_handler_) {
		event_schedule_handler_->on_schedule_update ();
	}
}

bool event_information_table_sched::parse (const section_info *p_comp_section, table* p_out_table)
{
	if (!p_comp_section || !p_out_table) {
		return false;
	}

	uint8_t *p = NULL; // work
	table* p_table = p_out_table;

	p_table->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();
	p_table->transport_stream_id = *p << 8 | *(p+1);
	p_table->original_network_id = *(p+2) << 8 | *(p+3);
	p_table->segment_last_section_number = *(p+4);
	p_table->last_table_id = *(p+5);

	p += EIT_FIX_LEN;

	int event_len = (int) (p_table->header.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN - EIT_FIX_LEN);
	if (event_len == 0 || event_len == EIT_EVENT_FIX_LEN) {
		// allow
	} else if (event_len < EIT_EVENT_FIX_LEN) {
		_UTL_LOG_W ("invalid EIT event (event_len=%d)", event_len);
		return false;
	}

	while (event_len > 0) {

		table::event ev ;

		ev.event_id = *p << 8 | *(p+1);
		memcpy (ev.start_time, p+2, 5);
		memcpy (ev.duration, p+7, 3);
		ev.running_status = (*(p+10) >> 5) & 0x07;
		ev.free_CA_mode = (*(p+10) >> 4) & 0x01;
		ev.descriptors_loop_length = (*(p+10) & 0x0f) << 8 | *(p+11);

		p += EIT_EVENT_FIX_LEN;

		int n = (int)ev.descriptors_loop_length;
		while (n > 0) {
			descriptor desc (p);
			if (!desc.is_valid) {
				_UTL_LOG_W ("invalid EIT desc");
				return false;
			}
			ev.descriptors.push_back (desc);
			n -= (2 + *(p + 1));
			p += (2 + *(p + 1));
		}

		event_len -= (EIT_EVENT_FIX_LEN + ev.descriptors_loop_length) ;
		if (event_len < 0) {
			_UTL_LOG_W ("invalid EIT event");
			return false;
		}

		p_table->events.push_back (ev);
	}

	return true;
}

void event_information_table_sched::append_table (table *p_table)
{
	if (!p_table) {
		return ;
	}

	std::lock_guard<std::recursive_mutex> lock (mutex_tables_);

	tables_.push_back (p_table);
}

void event_information_table_sched::release_tables (void)
{
	std::lock_guard<std::recursive_mutex> lock (mutex_tables_);

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

void event_information_table_sched::release_table (table *p_erase)
{
	if (!p_erase) {
		return;
	}

	std::lock_guard<std::recursive_mutex> lock (mutex_tables_);

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

void event_information_table_sched::dump_tables (void)
{
	std::lock_guard<std::recursive_mutex> lock (mutex_tables_);

	if (tables_.size() == 0) {
		return;
	}

	std::vector<table*>::const_iterator iter = tables_.begin(); 
	for (; iter != tables_.end(); ++ iter) {
		table *p_table = *iter;
		dump_table (p_table);
	}
}

void event_information_table_sched::dumpTables_event (void)
{
	std::lock_guard<std::recursive_mutex> lock (mutex_tables_);

	if (tables_.size() == 0) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<table*>::const_iterator iter = tables_.begin(); 
	for (; iter != tables_.end(); ++ iter) {
		table *p_table = *iter;
		dumpTable_event (p_table);
	}
}

void event_information_table_sched::dumpTables_simple (void)
{
	std::lock_guard<std::recursive_mutex> lock (mutex_tables_);

	if (tables_.size() == 0) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);

	std::vector<table*>::const_iterator iter = tables_.begin(); 
	for (; iter != tables_.end(); ++ iter) {
		table *p_table = *iter;
		dumpTable_simple (p_table);
	}
}

void event_information_table_sched::dump_table (const table* p_table) const
{
	if (!p_table) {
		return;
	}
	
	_UTL_LOG_I (__PRETTY_FUNCTION__);
	p_table->header.dump ();
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id                    [0x%02x]\n", p_table->header.table_id);
	_UTL_LOG_I ("service_id                  [0x%04x]\n", p_table->header.table_id_extension);
	_UTL_LOG_I ("transport_stream_id         [0x%04x]\n", p_table->transport_stream_id);
	_UTL_LOG_I ("original_network_id         [0x%04x]\n", p_table->original_network_id);
	_UTL_LOG_I ("segment_last_section_number [0x%02x]\n", p_table->segment_last_section_number);
	_UTL_LOG_I ("last_table_id               [0x%02x]\n", p_table->last_table_id);

	std::vector<table::event>::const_iterator iter_event = p_table->events.begin();
	for (; iter_event != p_table->events.end(); ++ iter_event) {
		_UTL_LOG_I ("\n--  event  --\n");
		_UTL_LOG_I ("event_id                [0x%04x]\n", iter_event->event_id);
		char sz_stime [32];
		memset (sz_stime, 0x00, sizeof(sz_stime));
		ts_arib_common::get_str_epoch (ts_arib_common::get_epoch_from_mjd (iter_event->start_time), "%Y/%m/%d %H:%M:%S", sz_stime, sizeof(sz_stime));
		_UTL_LOG_I ("start_time              [%s]\n", sz_stime);
		char sz_duration [32];
		memset (sz_duration, 0x00, sizeof(sz_duration));
		ts_arib_common::get_str_second (ts_arib_common::get_sec_from_bcd (iter_event->duration), sz_duration, sizeof(sz_duration));
		_UTL_LOG_I ("duration                [%s]\n", sz_duration);
		_UTL_LOG_I ("running_status          [0x%02x]\n", iter_event->running_status);
		_UTL_LOG_I ("free_CA_mode            [0x%02x]\n", iter_event->free_CA_mode);
		_UTL_LOG_I ("descriptors_loop_length [%d]\n", iter_event->descriptors_loop_length);

		std::vector<descriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
		for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {
			_UTL_LOG_I ("\n--  descriptor  --\n");
			descriptor_common::dump (iter_desc->tag, *iter_desc);
		}
	}

	_UTL_LOG_I ("\n");
}

void event_information_table_sched::dumpTable_event (const table* p_table) const
{
	if (!p_table) {
		return;
	}
	
//	_UTL_LOG_I (__PRETTY_FUNCTION__);
//	p_table->header.dump ();
//	_UTL_LOG_I ("========================================\n");

//	_UTL_LOG_I ("table_id                    [0x%02x]\n", p_table->header.table_id);
//	_UTL_LOG_I ("service_id                  [0x%04x]\n", p_table->header.table_id_extension);
//	_UTL_LOG_I ("transport_stream_id         [0x%04x]\n", p_table->transport_stream_id);
//	_UTL_LOG_I ("original_network_id         [0x%04x]\n", p_table->original_network_id);
//	_UTL_LOG_I ("segment_last_section_number [0x%02x]\n", p_table->segment_last_section_number);
//	_UTL_LOG_I ("last_table_id               [0x%02x]\n", p_table->last_table_id);

	std::vector<table::event>::const_iterator iter_event = p_table->events.begin();

	if (iter_event != p_table->events.end()) {
		_UTL_LOG_I ("========================================");
	}

	for (; iter_event != p_table->events.end(); ++ iter_event) {
//		_UTL_LOG_I ("\n--  event  --\n");
		_UTL_LOG_I ("--  event  --\n");
		_UTL_LOG_I ("event_id                [0x%04x]\n", iter_event->event_id);
		char sz_stime [32];
		memset (sz_stime, 0x00, sizeof(sz_stime));
		ts_arib_common::get_str_epoch (ts_arib_common::get_epoch_from_mjd (iter_event->start_time), "%Y/%m/%d %H:%M:%S", sz_stime, sizeof(sz_stime));
		_UTL_LOG_I ("start_time              [%s]\n", sz_stime);
		char sz_duration [32];
		memset (sz_duration, 0x00, sizeof(sz_duration));
		ts_arib_common::get_str_second (ts_arib_common::get_sec_from_bcd (iter_event->duration), sz_duration, sizeof(sz_duration));
		_UTL_LOG_I ("duration                [%s]\n", sz_duration);
//		_UTL_LOG_I ("running_status          [0x%02x]\n", iter_event->running_status);
//		_UTL_LOG_I ("free_CA_mode            [0x%02x]\n", iter_event->free_CA_mode);
//		_UTL_LOG_I ("descriptors_loop_length [%d]\n", iter_event->descriptors_loop_length);

		std::vector<descriptor>::const_iterator iter_desc = iter_event->descriptors.begin();
		for (; iter_desc != iter_event->descriptors.end(); ++ iter_desc) {
//			_UTL_LOG_I ("\n--  descriptor  --\n");
//			descriptor_common::dump (iter_desc->tag, *iter_desc);

			if (iter_desc->tag == DESC_TAG__SHORT_EVENT_DESCRIPTOR) {
				short_event_descriptor sd (*iter_desc);
				if (!sd.is_valid) {
					_UTL_LOG_W ("invalid ShortEventDescriptor");
					continue;
				}

				char aribstr [MAXSECLEN];
				memset (aribstr, 0x00, MAXSECLEN);
				AribToString (aribstr, (const char*)sd.event_name_char, (int)sd.event_name_length);
				_UTL_LOG_I ("event_name_char         [%s]\n", aribstr);
			}
		}
	}

//	_UTL_LOG_I ("\n");
}

void event_information_table_sched::dumpTable_simple (const table* p_table) const
{
	if (!p_table) {
		return;
	}
	
//	_UTL_LOG_I ("========================================\n");
//	p_table->header.dump ();

	char buf[128] = {0};
	snprintf (buf, sizeof(buf) -1, "event_id:");
	std::vector<table::event>::const_iterator iter_event = p_table->events.begin();
	for (; iter_event != p_table->events.end(); ++ iter_event) {
		int len = strlen (buf);
		if (len > 120) {
			_UTL_LOG_W ("event still continues");
			return;
		}
		snprintf (buf+len, sizeof(buf) -1, "[0x%04x]", iter_event->event_id);
	}

//	_UTL_LOG_I (
//		"svcid:[0x%04x] [%s] tsid:[0x%04x] org_nid:[0x%04x] seg_last:[0x%02x] last:[0x%02x] %s\n",
//		p_table->header.table_id_extension,
//		p_table->header.table_id == 0x4e ? "PF ,A " :
//			p_table->header.table_id == 0x4f ? "PF ,O " :
//			p_table->header.table_id >= 0x50 && p_table->header.table_id < 0x58 ? "Sched,A " :
//			p_table->header.table_id >= 0x58 && p_table->header.table_id < 0x60 ? "Sched,AE" :
//			p_table->header.table_id >= 0x60 && p_table->header.table_id < 0x68 ? "Sched,O " :
//			p_table->header.table_id >= 0x68 && p_table->header.table_id < 0x70 ? "Sched,OE" :
//			"unsup ",
//		p_table->transport_stream_id,
//		p_table->original_network_id,
//		p_table->segment_last_section_number,
//		p_table->last_table_id,
//		buf
//	);
	_UTL_LOG_I (
		"tblid:[0x%02x %s] syntax:[0x%02x] len:[%6d] svcid:[0x%04x] ver:[0x%02x] next:[0x%02x] num:[0x%02x] last:[0x%02x] tsid:[0x%04x] org_nid:[0x%04x] seg_last:[0x%02x] last:[0x%02x] %s\n",
		p_table->header.table_id,
		p_table->header.table_id == 0x4e ? "PF ,A " :
			p_table->header.table_id == 0x4f ? "PF ,O " :
			p_table->header.table_id >= 0x50 && p_table->header.table_id < 0x58 ? "Sched,A " :
			p_table->header.table_id >= 0x58 && p_table->header.table_id < 0x60 ? "Sched,AE" :
			p_table->header.table_id >= 0x60 && p_table->header.table_id < 0x68 ? "Sched,O " :
			p_table->header.table_id >= 0x68 && p_table->header.table_id < 0x70 ? "Sched,OE" :
			"unsup ",
		p_table->header.section_syntax_indicator,
		p_table->header.section_length,
		p_table->header.table_id_extension,
		p_table->header.version_number,
		p_table->header.current_next_indicator,
		p_table->header.section_number,
		p_table->header.last_section_number,
		p_table->transport_stream_id,
		p_table->original_network_id,
		p_table->segment_last_section_number,
		p_table->last_table_id,
		buf
	);
}

void event_information_table_sched::clear (void)
{
	release_tables ();

//	detach_all_section_list ();
	// detach_all_section_list in parser loop
	async_delete ();
}

void event_information_table_sched::clear (table *p_erase)
{
	release_table (p_erase);
}

bool event_information_table_sched::is_duplicate_sub_tables (table* p_table)
{
	if (!p_table) {
		return false;
	}

	std::lock_guard<std::recursive_mutex> lock (mutex_tables_);


	// sub-table identify
	uint8_t _tblid = p_table->header.table_id;
	uint16_t _svcid = p_table->header.table_id_extension;
	uint16_t _tsid = p_table->transport_stream_id;
	uint16_t _org_nid = p_table->original_network_id;
	uint8_t _num = p_table->header.section_number;
	uint8_t _version = p_table->header.version_number;


	std::vector<table*>::const_iterator iter = tables_.begin();
    for (; iter != tables_.end(); ++ iter) {
		table *p = *iter;

		// check every sub-tables
		if (
			(_tblid == p->header.table_id) &&
			(_svcid == p->header.table_id_extension) &&
			(_tsid == p->transport_stream_id) &&
			(_org_nid == p->original_network_id) &&
			(_num == p->header.section_number) &&
			(_version == p->header.version_number)
		) {
			return true;
		}
	}

	return false;
}

bool event_information_table_sched::refresh_sub_tables_by_version_number (table* p_new_table)
{
	if (!p_new_table) {
		return false;
	}

	std::lock_guard<std::recursive_mutex> lock (mutex_tables_);


	// sub-table identify
	uint8_t new_tblid = p_new_table->header.table_id;
	uint16_t new_svcid = p_new_table->header.table_id_extension;
	uint16_t new_tsid = p_new_table->transport_stream_id;
	uint16_t new_org_nid = p_new_table->original_network_id;
	uint8_t new_ver = p_new_table->header.version_number;
	uint8_t new_sec_num = p_new_table->header.section_number;

	table *p_erase = NULL;

	std::vector<table*>::const_iterator iter = tables_.begin();
    for (; iter != tables_.end(); ++ iter) {
		table *p_table = *iter;

		// check every sub-tables
		if (
			(new_tblid == p_table->header.table_id) &&
			(new_svcid == p_table->header.table_id_extension) &&
			(new_tsid == p_table->transport_stream_id) &&
			(new_org_nid == p_table->original_network_id) &&
			(new_sec_num == p_table->header.section_number)
		) {
			// EIT schedule も「最新受信」を採用する。
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
			"EITsched new ver >> tblid:[0x%02x] svcid:[0x%04x] tsid:[0x%04x] org_nid:[0x%04x] num:[0x%02x] ver:[0x%02x]->[0x%02x]",
			new_tblid,
			new_svcid,
			new_tsid,
			new_org_nid,
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

void event_information_table_sched::refresh_sub_tables (table* p_new_table)
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

event_information_table_sched::reference event_information_table_sched::get_reference (void)
{
	reference ref (&tables_, &mutex_tables_);
	return ref;
}


} // namespace parser
