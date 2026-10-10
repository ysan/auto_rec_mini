#ifndef _EVENT_INFORMATION_TABLE_H_
#define _EVENT_INFORMATION_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
//#include <mutex>

#include "defs.h"
#include "ts_arib_common.h"
#include "section_parser.h"
#include "descriptor_defs.h"

namespace parser {



#define EIT_FIX_LEN			(6) // transport_stream_id         16  uimsbf
								// original_network_id         16  uimsbf
								// segment_last_section_number  8  uimsbf
								// last_table_id                8  uimsbf

#define EIT_EVENT_FIX_LEN	(12) // event_id                16  uimsbf
								// start_time               40  mjd
								// duration                 24  bcdtime
								// running_status            3  uimsbf
								// free_CA_mode              1  bslbf
								// descriptors_loop_length  12  uimsbf


class event_information_table : public section_parser
{
public:
	class table {
	public:
		class event {
		public:
			event (void)
				:event_id (0)
				,running_status (0)
				,free_CA_mode (0)
				,descriptors_loop_length (0)
			{
				memset (start_time, 0x00, sizeof(start_time));
				memset (duration, 0x00, sizeof(duration));
				descriptors.clear();
			}
			virtual ~event (void)
			{
				descriptors.clear();
			}

			uint16_t event_id;
			uint8_t start_time [8];
			uint8_t duration [3];
			uint8_t running_status;
			uint8_t free_CA_mode;
			uint16_t descriptors_loop_length;
			std::vector <descriptor> descriptors;
		};

	public:
		table (void)
			:transport_stream_id (0)
			,original_network_id (0)
			,segment_last_section_number (0)
			,last_table_id (0)
		{
			events.clear();
		}
		virtual ~table (void)
		{
			events.clear();
		}

		ST_SECTION_HEADER header;
		uint16_t transport_stream_id;
		uint16_t original_network_id;
		uint8_t segment_last_section_number;
		uint8_t last_table_id;
		std::vector <event> events;
	};

public:
//	class reference {
//	public:
//		reference (void) {}
//		reference (const std::vector <table*> *p_tables, std::recursive_mutex *p_mutex)
//			:tables_ (p_tables)
//			,mutex_ (p_mutex)
//		{}
//		virtual ~reference (void) {}
//
//		const std::vector <table*> *tables_;
//		std::recursive_mutex *mutex_;
//	};

public:
	explicit event_information_table (size_t pool_size);
	event_information_table (size_t pool_size, int fifo_num);
	virtual ~event_information_table (void);


	// section_parser
	bool on_section_started (const section_info *p_section) override;
	void on_section_completed (const section_info *p_comp_section) override;

	void dump_tables (void);
	void dumpTables_simple (void);

	void dump_table (const table* p_table) const;
	void dumpTable_simple (const table* p_table) const;

	void clear (void);
	void clear (table* p_erase);

	std::vector <table*> *get_tables (void) {
		return &tables_;
	}

//	reference reference (void);

private:
	bool parse (const section_info *p_comp_section, table* p_out_table);
	void append_table (table *p_table);
	void release_tables (void);
	void release_table (table* p_erase);

	bool refresh_sub_tables_by_version_number (table* p_new_table);
	void refresh_sub_tables (table* p_new_table);


	std::vector <table*> tables_;
//	std::recursive_mutex mutex_tables_;
};


} // namespace parser

#endif
