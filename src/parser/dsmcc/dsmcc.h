#ifndef _DSMCC_H_
#define _DSMCC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "defs.h"
#include "ts_arib_common.h"
#include "section_parser.h"
#include "dsmcc_descriptor_defs.h"

namespace parser {



class dsmcc_adaptation_header
{
public:
	dsmcc_adaptation_header (void)
		:adaptation_type (0)
	{
		memset (adaptation_data_byte, 0x00, sizeof(adaptation_data_byte));
	}
	virtual ~dsmcc_adaptation_header (void) {}


	uint8_t adaptation_type;
	uint8_t adaptation_data_byte [0xff];
};

class dsmcc_message_header
{
public:
	dsmcc_message_header (void)
		:protocol_discriminator (0)
		,dsmcc_type (0)
		,message_id (0)
		,transaction_id (0)
		,reserved (0)
		,adaptation_length (0)
		,message_length (0)
	{}
	virtual ~dsmcc_message_header (void) {}


	uint8_t protocol_discriminator;
	uint8_t dsmcc_type;
	uint16_t message_id;
	uint32_t transaction_id;
	uint8_t reserved;
	uint8_t adaptation_length;
	uint16_t message_length;
	dsmcc_adaptation_header adaptation_header;
};

// DII
class download_info_indication
{
public:
	class module {
	public:
		module (void)
			:module_id (0)
			,module_size (0)
			,module_version (0)
			,module_info_length (0)
		{
			memset (module_info_byte, 0x00, sizeof(module_info_byte));
		}
		virtual ~module (void) {}

		uint16_t module_id;
		uint32_t module_size;
		uint8_t module_version;
		uint8_t module_info_length;
		uint8_t module_info_byte [0xff];
	};

public:
	download_info_indication (void)
		:download_id (0)
		,block_size (0)
		,window_size (0)
		,ack_period (0)
		,tc_download_window (0)
		,tc_download_scenario (0)
		,number_of_modules (0)
		,private_data_length (0)
	{
		modules.clear();
		memset (private_data_byte, 0x00, sizeof(private_data_byte));
	}
	virtual ~download_info_indication (void)
	{
		modules.clear();
	}

	
	dsmcc_message_header message_header;
	uint32_t download_id;
	uint16_t block_size;
	uint8_t window_size;
	uint8_t ack_period;
	uint32_t tc_download_window;
	uint32_t tc_download_scenario;
	// compatibility_descriptor() // 4bytes?
	uint16_t number_of_modules;
	std::vector <module> modules;
	uint16_t private_data_length;
	uint8_t private_data_byte [0xffff];

};

class dsmcc_download_data_header
{
public:
	dsmcc_download_data_header (void)
		:protocol_discriminator (0)
		,dsmcc_type (0)
		,message_id (0)
		,download_id (0)
		,reserved (0)
		,adaptation_length (0)
		,message_length (0)
	{}
	virtual ~dsmcc_download_data_header (void) {}

	uint8_t protocol_discriminator;
	uint8_t dsmcc_type;
	uint16_t message_id;
	uint32_t download_id;
	uint8_t reserved;
	uint8_t adaptation_length;
	uint16_t message_length;
	dsmcc_adaptation_header adaptation_header;
};

#define DDB_FIX_LEN		(6) // module_id      16  uimsbf
							// module_version  8  uimsbf
							// reserved       8  uimsbf
							// block_number   16  uimsbf

// DDB
class download_data_block
{
public:
	download_data_block (void)
		:module_id (0)
		,module_version (0)
		,reserved (0)
		,block_number (0)
		,m_blockDataByteLen (0)
	{
		memset (block_data_byte, 0x00, sizeof(block_data_byte));
	}
	virtual ~download_data_block (void) {}

	dsmcc_download_data_header download_data_header;
	uint16_t module_id;
	uint8_t module_version;
	uint8_t reserved;
	uint16_t block_number;
	uint8_t block_data_byte [0xffff];

	int m_blockDataByteLen; // for block_data_byte
};


class dsmcc : public section_parser
{
public:
	class dii {
	public:
		dii (void) {}
		virtual ~dii (void) {}

		ST_SECTION_HEADER header;
		download_info_indication body;
	};

	class ddb {
	public:
		ddb (void) {}
		virtual ~ddb (void) {}

		ST_SECTION_HEADER header;
		download_data_block body;
	};

public:
	explicit dsmcc (size_t pool_size);
	virtual ~dsmcc (void);

	bool on_section_started (const section_info *p_section) override;
	void on_section_completed (const section_info *p_comp_section) override;

	void dump_diis (void) const;
	void dump_ddbs (void) const;

	void dump_dii (const dii* p_dii) const;
	void dump_ddb (const ddb* p_ddb) const;

	void clear (void);


private:
	bool parse_dii (const section_info *p_comp_section, dii* p_out_dii);
	bool parse_ddb (const section_info *p_comp_section, ddb* p_out_ddb);

	void release_diis (void);
	void release_ddbs (void);

	std::vector <dii*> diis_;
	std::vector <ddb*> ddbs_;
};


} // namespace parser

#endif
