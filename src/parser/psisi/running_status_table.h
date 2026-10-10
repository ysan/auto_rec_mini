#ifndef _RUNNING_STATUS_TABLE_H_
#define _RUNNING_STATUS_TABLE_H_

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



#define RST_STATUS_FIX_LEN	(9) // transport_stream_id   16  uimsbf
								// original_network_id   16  uimsbf
								// service_id            16  uimsbf
								// event_id              16  uimsbf
								// reserved_future_use    5  bslbf
								// running_status         3  uimsbf


class running_status_table : public section_parser
{
public:
	class table {
	public:
		class status {
		public:
			status (void)
				:transport_stream_id (0)
				,original_network_id (0)
				,service_id (0)
				,event_id (0)
				,running_status (0) {}
			virtual ~status (void) {}

			uint16_t transport_stream_id;
			uint16_t original_network_id;
			uint16_t service_id;
			uint16_t event_id;
			uint8_t running_status;
		};

	public:
		table (void)
		{
			statuses.clear();
		}
		virtual ~table (void)
		{
			statuses.clear();
		}

		ST_SECTION_HEADER header;
		std::vector <status> statuses;
	};

public:
	running_status_table (void);
	explicit running_status_table (int fifo_num);
	virtual ~running_status_table (void);


	// section_parser
	void on_section_completed (const section_info *p_comp_section) override;

	void dump_tables (void);
	void dump_table (const table* p_table) const;
	void clear (void);

	std::vector <table*> * get_tables (void) {
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
