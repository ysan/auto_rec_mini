#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "dsmcc.h"
#include "utils.h"

namespace parser {



dsmcc::dsmcc (size_t pool_size)
	:section_parser (pool_size, EN_SECTION_TYPE__DSMCC)
{
	diis_.clear();
	ddbs_.clear();
}

dsmcc::~dsmcc (void)
{
	clear();
}


bool dsmcc::on_section_started (const section_info *p_comp_section)
{
	if (!p_comp_section) {
		return false;
	}

	ST_SECTION_HEADER const *p_hdr = const_cast<section_info*>(p_comp_section)->get_header ();
	if (!p_hdr) {
		return false;
	}

	if ((p_hdr->table_id == 0x3b) || (p_hdr->table_id == 0x3c)) {
		return true;
	}

	_UTL_LOG_W ("dsmcc: unsupported table_id:[0x%02x]", p_hdr->table_id);
	return false;
}

void dsmcc::on_section_completed (const section_info *p_comp_section)
{
	if (!p_comp_section) {
		return ;
	}

	ST_SECTION_HEADER const *p_hdr = const_cast<section_info*>(p_comp_section)->get_header ();
	if (!p_hdr) {
		return ;
	}

	if (p_hdr->table_id == 0x3b) {

		dii *p_dii = new dii ();
		if (!parse_dii (p_comp_section, p_dii)) {
			delete p_dii;
			p_dii = NULL;
			detach_section_list (p_comp_section);
			return ;
		}

		diis_.push_back (p_dii);
		dump_dii (p_dii);

//TODO
//detach_section_list (p_comp_section);

	} else if (p_hdr->table_id == 0x3c) {

		ddb *p_ddb = new ddb ();
		if (!parse_ddb (p_comp_section, p_ddb)) {
			delete p_ddb;
			p_ddb = NULL;
			detach_section_list (p_comp_section);
			return ;
		}

		ddbs_.push_back (p_ddb);
		dump_ddb (p_ddb);

//TODO
//detach_section_list (p_comp_section);

	} else {
		_UTL_LOG_W ("dsmcc: unexpected table_id:[0x%02x]", p_hdr->table_id);
		detach_section_list (p_comp_section);
	}
}

bool dsmcc::parse_dii (const section_info *p_comp_section, dii* p_out_dii)
{
	if (!p_comp_section || !p_out_dii) {
		return false;
	}

	uint8_t *p = NULL; // work
	dii* p_dii = p_out_dii;

	p_dii->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();

	p_dii->body.message_header.protocol_discriminator = *p;
	p += 1;
	p_dii->body.message_header.dsmcc_type = *p;
	p += 1;
	p_dii->body.message_header.message_id = *p << 8 | *(p+1);
	p += 2;
	p_dii->body.message_header.transaction_id = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	p_dii->body.message_header.reserved = *p;
	p += 1;
	p_dii->body.message_header.adaptation_length = *p;
	p += 1;
	p_dii->body.message_header.message_length = *p << 8 | *(p+1);
	p += 2;
	int adapt_len = p_dii->body.message_header.adaptation_length;
	if (adapt_len > 0) {
		p_dii->body.message_header.adaptation_header.adaptation_type = *p;
		memcpy (p_dii->body.message_header.adaptation_header.adaptation_data_byte, p+1, adapt_len -1);
		p += adapt_len;
	}

	p_dii->body.download_id = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	p_dii->body.block_size = *p << 8 | *(p+1);
	p += 2;
	p_dii->body.window_size = *p;
	p += 1;
	p_dii->body.ack_period = *p;
	p += 1;
	p_dii->body.tc_download_window = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	p_dii->body.tc_download_scenario = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;

	// compatibility_descriptor() // 4bytes?
	uint16_t compatibility_descriptor_length = *p << 8 | *(p+1);
	int cd_len = compatibility_descriptor_length;
	if (cd_len > 0) {
		p += 2 + cd_len;
	} else {
		p += 4;
	}

	p_dii->body.number_of_modules = *p << 8 | *(p+1);
	p += 2;
	int module_num = p_dii->body.number_of_modules;
	while (module_num > 0) {
		download_info_indication::module mod;
		mod.module_id = *p << 8 | *(p+1);
		p += 2;
		mod.module_size = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
		p += 4;
		mod.module_version = *p;
		p += 1;
		mod.module_info_length = *p;
		p += 1;
		memcpy (mod.module_info_byte, p, mod.module_info_length);
		p += (int)mod.module_info_length;
		p_dii->body.modules.push_back (mod);
		-- module_num;
	}
	
	p_dii->body.private_data_length = *p << 8 | *(p+1);
	p += 2;
	memcpy (p_dii->body.private_data_byte, p, (int)p_dii->body.private_data_length);
	p += (int)p_dii->body.private_data_length;

	// DII length check
	int total_len = p - p_comp_section->get_data_part_addr ();
	if (total_len != p_comp_section->get_data_part_len ()) {
		_UTL_LOG_W ("dsmcc: DII length is invalid... [%d:%d]", total_len, p_comp_section->get_data_part_len ());
		return false;
	}

	return true;
}

bool dsmcc::parse_ddb (const section_info *p_comp_section, ddb* p_out_ddb)
{
	if (!p_comp_section || !p_out_ddb) {
		return false;
	}

	uint8_t *p = NULL; // work
	ddb* p_ddb = p_out_ddb;

	p_ddb->header = *(const_cast<section_info*>(p_comp_section)->get_header ());

	p = p_comp_section->get_data_part_addr ();

	p_ddb->body.download_data_header.protocol_discriminator = *p;
	p += 1;
	p_ddb->body.download_data_header.dsmcc_type = *p;
	p += 1;
	p_ddb->body.download_data_header.message_id = *p << 8 | *(p+1);
	p += 2;
	p_ddb->body.download_data_header.download_id = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	p_ddb->body.download_data_header.reserved = *p;
	p += 1;
	p_ddb->body.download_data_header.adaptation_length = *p;
	p += 1;
	p_ddb->body.download_data_header.message_length = *p << 8 | *(p+1);
	p += 2;
	int adapt_len = p_ddb->body.download_data_header.adaptation_length;
	if (adapt_len > 0) {
		p_ddb->body.download_data_header.adaptation_header.adaptation_type = *p;
		memcpy (p_ddb->body.download_data_header.adaptation_header.adaptation_data_byte, p+1, adapt_len -1);
		p += adapt_len;
	}

	p_ddb->body.module_id = *p << 8 | *(p+1);
	p += 2;
	p_ddb->body.module_version = *p;
	p += 1;
	p_ddb->body.reserved = *p;
	p += 1;
	p_ddb->body.block_number = *p << 8 | *(p+1);
	p += 2;
	p_ddb->body.m_blockDataByteLen = p_ddb->body.download_data_header.message_length - adapt_len - DDB_FIX_LEN;
	if (p_ddb->body.m_blockDataByteLen < 0) {
		_UTL_LOG_W ("dsmcc: m_blockDataByteLen invalid [%d]", p_ddb->body.m_blockDataByteLen);
		return false;
	}
	memcpy (p_ddb->body.block_data_byte, p, p_ddb->body.m_blockDataByteLen);
	p += p_ddb->body.m_blockDataByteLen;

	// DDB length check
	int total_len = p - p_comp_section->get_data_part_addr ();
	if (total_len != p_comp_section->get_data_part_len ()) {
		_UTL_LOG_W ("dsmcc: DDB length is invalid... [%d:%d]", total_len, p_comp_section->get_data_part_len ());
		return false;
	}

	return true;
}

void dsmcc::release_diis (void)
{
	if (diis_.size() == 0) {
		return;
	}

	std::vector<dii*>::iterator iter = diis_.begin(); 
	for (; iter != diis_.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	diis_.clear();
}

void dsmcc::release_ddbs (void)
{
	if (ddbs_.size() == 0) {
		return;
	}

	std::vector<ddb*>::iterator iter = ddbs_.begin(); 
	for (; iter != ddbs_.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	ddbs_.clear();
}

void dsmcc::dump_diis (void) const
{
	if (diis_.size() == 0) {
		return;
	}

	std::vector<dii*>::const_iterator iter = diis_.begin(); 
	for (; iter != diis_.end(); ++ iter) {
		dii *p_dii = *iter;
		dump_dii (p_dii);
	}
}

void dsmcc::dump_ddbs (void) const
{
	if (ddbs_.size() == 0) {
		return;
	}

	std::vector<ddb*>::const_iterator iter = ddbs_.begin(); 
	for (; iter != ddbs_.end(); ++ iter) {
		ddb *p_ddb = *iter;
		dump_ddb (p_ddb);
	}
}

void dsmcc::dump_dii (const dii* p_dii) const
{
	if (!p_dii) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id           [0x%02x]\n", p_dii->header.table_id);

	_UTL_LOG_I ("dsmcc_message_header.protocol_discriminator                [0x%02x]\n", p_dii->body.message_header.protocol_discriminator);
	_UTL_LOG_I ("dsmcc_message_header.dsmcc_type                            [0x%02x]\n", p_dii->body.message_header.dsmcc_type);
	_UTL_LOG_I ("dsmcc_message_header.message_id                            [0x%04x]\n", p_dii->body.message_header.message_id);
	_UTL_LOG_I ("dsmcc_message_header.transaction_id                       [0x%08x]\n", p_dii->body.message_header.transaction_id);
	_UTL_LOG_I ("dsmcc_message_header.adaptation_length                     [%d]\n", p_dii->body.message_header.adaptation_length);
	_UTL_LOG_I ("dsmcc_message_header.message_length                        [%d]\n", p_dii->body.message_header.message_length);
	int adapt_len = p_dii->body.message_header.adaptation_length;
	if (adapt_len > 0) {
		_UTL_LOG_I ("dsmcc_message_header.adaptation_header.adaptation_type [0x%02x]\n", p_dii->body.message_header.adaptation_header.adaptation_type);
		arec::common::utils::dumper (p_dii->body.message_header.adaptation_header.adaptation_data_byte, p_dii->body.message_header.adaptation_length);
	}

	_UTL_LOG_I ("download_id         [0x%08x]\n", p_dii->body.download_id);
	_UTL_LOG_I ("block_size          [%d]\n", p_dii->body.block_size);
	_UTL_LOG_I ("window_size         [%d]\n", p_dii->body.window_size);
	_UTL_LOG_I ("ack_period          [0x%02x]\n", p_dii->body.ack_period);
	_UTL_LOG_I ("tc_download_window   [0x%08x]\n", p_dii->body.tc_download_window);
	_UTL_LOG_I ("tc_download_scenario [0x%08x]\n", p_dii->body.tc_download_scenario);
	_UTL_LOG_I ("number_of_modules    [%d]\n", p_dii->body.number_of_modules);

	std::vector<download_info_indication::module>::const_iterator iter_mod = p_dii->body.modules.begin();
	for (; iter_mod != p_dii->body.modules.end(); ++ iter_mod) {
		_UTL_LOG_I ("\n--  module  --\n");
        _UTL_LOG_I ("module_id         [0x%04x]\n", iter_mod->module_id);
        _UTL_LOG_I ("module_size       [%d]\n", iter_mod->module_size);
        _UTL_LOG_I ("module_version    [0x%02x]\n", iter_mod->module_version);
        _UTL_LOG_I ("module_info_length [%d]\n", iter_mod->module_info_length);
		arec::common::utils::dumper (iter_mod->module_info_byte, iter_mod->module_info_length);
	}

	_UTL_LOG_I ("private_data_length  [%d]\n", p_dii->body.private_data_length);
	if ((int)p_dii->body.private_data_length > 0) {
		arec::common::utils::dumper (p_dii->body.private_data_byte, p_dii->body.private_data_length);
	}

	_UTL_LOG_I ("\n");
}

void dsmcc::dump_ddb (const ddb* p_ddb) const
{
	if (!p_ddb) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id         [0x%02x]\n", p_ddb->header.table_id);

	_UTL_LOG_I ("dsmcc_download_data_header.protocol_discriminator                [0x%02x]\n", p_ddb->body.download_data_header.protocol_discriminator);
	_UTL_LOG_I ("dsmcc_download_data_header.dsmcc_type                            [0x%02x]\n", p_ddb->body.download_data_header.dsmcc_type);
	_UTL_LOG_I ("dsmcc_download_data_header.message_id                            [0x%04x]\n", p_ddb->body.download_data_header.message_id);
	_UTL_LOG_I ("dsmcc_download_data_header.download_id                           [0x%08x]\n", p_ddb->body.download_data_header.download_id);
	_UTL_LOG_I ("dsmcc_download_data_header.adaptation_length                     [%d]\n", p_ddb->body.download_data_header.adaptation_length);
	_UTL_LOG_I ("dsmcc_download_data_header.message_length                        [%d]\n", p_ddb->body.download_data_header.message_length);
	int adapt_len = p_ddb->body.download_data_header.adaptation_length;
	if (adapt_len > 0) {
		_UTL_LOG_I ("dsmcc_download_data_header.adaptation_header.adaptation_type [0x%02x]\n", p_ddb->body.download_data_header.adaptation_header.adaptation_type);
		arec::common::utils::dumper (p_ddb->body.download_data_header.adaptation_header.adaptation_data_byte, p_ddb->body.download_data_header.adaptation_length);
	}

	_UTL_LOG_I ("module_id         [0x%04x]\n", p_ddb->body.module_id);
	_UTL_LOG_I ("module_version    [0x%02x]\n", p_ddb->body.module_version);
	_UTL_LOG_I ("block_number      [%d]\n", p_ddb->body.block_number);
	if (p_ddb->body.m_blockDataByteLen > 0) {
		arec::common::utils::dumper (p_ddb->body.block_data_byte, p_ddb->body.m_blockDataByteLen);
	}

	_UTL_LOG_I ("\n");
}

void dsmcc::clear (void)
{
	release_ddbs ();
	release_diis ();
	detach_all_section_list ();
}


} // namespace parser
