#ifndef _NETWORK_INFORMATION_TABLE_H_
#define _NETWORK_INFORMATION_TABLE_H_

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



#define NIT_FIX_LEN			(2) // reserved_future_use_2        4  bslbf
								// network_descriptors_length  12  uimsbf

#define NIT_FIX2_LEN		(2) // reserved_future_use_3          4  bslbf
								// transport_stream_loop_length  12  uimsbf

#define NIT_STREAM_FIX_LEN	(6) // transport_stream_id      16  uimsbf
								// original_network_id      16  uimsbf
								// reserved_future_use       4  bslbf
								// descriptors_loop_length  12  uimsbf


class network_information_table : public section_parser
{
public:
	class table {
	public:
		class stream {
		public:
			stream (void)
				:transport_stream_id (0)
				,original_network_id (0)
				,reserved_future_use (0)
				,transport_descriptors_length (0)
			{
				descriptors.clear();
			}
			virtual ~stream (void)
			{
				descriptors.clear();
			}

			uint16_t transport_stream_id;
			uint16_t original_network_id;
			uint8_t reserved_future_use;
			uint16_t transport_descriptors_length;
			std::vector <descriptor> descriptors;
		};

	public:
		table (void)
			:reserved_future_use_2 (0)
			,network_descriptors_length (0)
			,reserved_future_use_3 (0)
			,transport_stream_loop_length (0)
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
		uint8_t reserved_future_use_2;
		uint16_t network_descriptors_length;
		std::vector <descriptor> descriptors;
		uint8_t reserved_future_use_3;
		uint16_t transport_stream_loop_length;
		std::vector <stream> streams;
	};

public:
	network_information_table (void);
	explicit network_information_table (int fifo_num);
	virtual ~network_information_table (void);


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
