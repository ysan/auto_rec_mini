#ifndef _PROGRAM_MAP_TABLE_H_
#define _PROGRAM_MAP_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "defs.h"
#include "ts_arib_common.h"
#include "section_parser.h"
#include "descriptor_defs.h"

namespace parser {



#define PMT_FIX_LEN				(4) // reserved             3  bslbf
									// PCR_PID             13  uimsbf
									// reserved             4  bslbf
									// program_info_length 12  uimsbf

#define PMT_STREAM_FIX_LEN		(5) // stream_type          8  uimsbf
									// reserved             3  bslbf
									// elementary_PID      13  uimsbf
									// reserved             4  bslbf
									// ES_info_length      12  uimsbf


class program_map_table : public section_parser
{
public:
	class table {
	public:
		class stream {
		public:
			stream (void)
				:stream_type (0)
				,reserved_1 (0)
				,elementary_PID (0)
				,reserved_2 (0)
				,ES_info_length (0)
			{
				descriptors.clear();
			}
			virtual ~stream (void)
			{
				descriptors.clear();
			}

			uint8_t stream_type;
			uint8_t reserved_1;
			uint16_t elementary_PID;
			uint8_t reserved_2;
			uint16_t ES_info_length;
			std::vector <descriptor> descriptors;
		};

	public:
		table (void)
			:reserved_3 (0)
			,PCR_PID (0)
			,reserved_4 (0)
			,program_info_length (0)
		{
			descriptors.clear();
			streams.clear();
		}
		virtual ~table (void)
		{
			descriptors.clear();
			streams.clear();
		}

		ST_SECTION_HEADER header;
		uint8_t reserved_3;
		uint16_t PCR_PID;
		uint8_t reserved_4;
		uint16_t program_info_length;
		std::vector <descriptor> descriptors;
		std::vector <stream> streams;
	};

public:
	program_map_table (void);
	explicit program_map_table (int fifio_num);
	virtual ~program_map_table (void);


	// section_parser
	void on_section_completed (const section_info *p_comp_section) override;

	void dump_tables (void);
	void dump_table (const table* p_table) const;
	void clear (void);
	void force_clear (void);

	std::vector <table*> *get_tables (void) {
		return & tables_;
	}

private:
	bool parse (const section_info *p_comp_section, table* p_out_table);
	void append_table (table *p_table);
	void release_tables (void);


	std::vector <table*> tables_;
};


} // namespace parser

#endif
