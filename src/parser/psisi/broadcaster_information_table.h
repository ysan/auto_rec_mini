#ifndef _BROADCASTER_INFORMATION_TABLE_H_
#define _BROADCASTER_INFORMATION_TABLE_H_

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



#define BIT_FIX_LEN				(2) // reserved_future_use        3  bslbf
									// broadcast_view_propriety   1  uimsbf
									// first_descriptors_length  12  uimsbf

#define BIT_BROADCASTER_FIX_LEN	(3) // broadcaster_id                   8  uimsbf
									// reserved_future_use              4  bslbf
									// broadcaster_descriptors_length  12  uimsbf


class broadcaster_information_table : public section_parser
{
public:
	class table {
	public:
		class broadcaster {
		public:
			broadcaster (void)
				:broadcaster_id (0)
				,reserved_future_use (0)
				,broadcaster_descriptors_length (0)
			{
				descriptors.clear();
			}
			virtual ~broadcaster (void)
			{
				descriptors.clear();
			}

			uint8_t broadcaster_id;
			uint8_t reserved_future_use;
			uint16_t broadcaster_descriptors_length;
			std::vector <descriptor> descriptors;
		};

	public:
		table (void)
			:reserved_future_use_2 (0)
			,broadcast_view_propriety (0)
			,first_descriptors_length (0)
		{
			descriptors.clear();
			broadcasters.clear();
		}
		virtual ~table (void)
		{
			descriptors.clear();
			broadcasters.clear();
		}

		ST_SECTION_HEADER header;
		uint8_t reserved_future_use_2;
		uint8_t broadcast_view_propriety;
		uint16_t first_descriptors_length;
		std::vector <descriptor> descriptors;
		std::vector <broadcaster> broadcasters;
	};

public:
	broadcaster_information_table (void);
	explicit broadcaster_information_table (int fifo_num);
	virtual ~broadcaster_information_table (void);


	// section_parser
	void on_section_completed (const section_info *p_comp_section) override;

	void dump_tables (void);
	void dump_table (const table* p_table) const;
	void clear (void);

	std::vector <table*> *get_tables (void) {
		return &tables_;
	}

private:
	bool parse (const section_info *p_comp_section, table* p_out_table);
	void append_table (table *p_table);
	void release_tables (void);


	std::vector <table*> tables_;
};


} // namespace parser

#endif
