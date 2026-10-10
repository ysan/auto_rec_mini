#ifndef _COMMON_DATA_TABLE_H_
#define _COMMON_DATA_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <memory>

#include "defs.h"
#include "ts_arib_common.h"
#include "section_parser.h"
#include "descriptor_defs.h"

namespace parser {



#define CDT_FIX_LEN			(5) // original_etwork_id          16  uimsbf
								// data_type                    8  uimsbf
								// reserved_future_use          4  bslbf
								// descriptors_loop_length     12  uimsbf

#define CDT_DATA_FIX_LEN	(7) // logo_type                    8  uimsbf
								// reserved_future_use          7  bslbf
								// logo_id                      9  uimsbf
								// reserved_future_use          4  bslbf
								// logo_version                12  uimsbf
								// data_size                   16  uimsbf


class common_data_table : public section_parser
{
public:
	class table {
	public:
		class data {
		public:
			data (void)
				:logo_type (0)
				,reserved_future_use_1 (0)
				,logo_id (0)
				,reserved_future_use_2 (0)
				,logo_version (0)
				,data_size (0)
			{
				data_byte.reset();
			}
			virtual ~data (void)
			{
				data_byte.reset();
			}

			uint8_t logo_type;
			uint8_t reserved_future_use_1;
			uint16_t logo_id;
			uint8_t reserved_future_use_2;
			uint16_t logo_version;
			uint16_t data_size;
			std::unique_ptr<uint8_t[]> data_byte;
		};

	public:
		table (void)
			:original_network_id (0)
			,data_type (0)
			,reserved_future_use (0)
			,descriptors_loop_length (0)
		{
			descriptors.clear();
		}
		virtual ~table (void)
		{
			descriptors.clear();
		}

		ST_SECTION_HEADER header;
		uint16_t original_network_id;
		uint8_t data_type;
		uint8_t reserved_future_use;
		uint16_t descriptors_loop_length;
		std::vector <descriptor> descriptors;
		data data;
	};

public:
	common_data_table (void);
	explicit common_data_table (int fifo_num);
	explicit common_data_table (size_t pool_size);
	virtual ~common_data_table (void);


	// section_parser
	void on_section_completed (const section_info *p_comp_section) override;

	void dump_tables (void);
	void dump_table (const table* p_table) const;
	void clear (void);

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
