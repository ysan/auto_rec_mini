#ifndef _PROGRAM_ASSOCIATION_TABLE_H_
#define _PROGRAM_ASSOCIATION_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "defs.h"
#include "ts_arib_common.h"
#include "section_parser.h"

namespace parser {



class program_association_table : public section_parser
{
public:
	class table {
	public:
		class program {
		public:
			program (void)
				:program_number (0)
				,program_map_PID (0)
			{}
			virtual ~program (void) {}

			uint16_t program_number;
			uint16_t program_map_PID; // if program_number == 0 then network_PID
		};

	public:
		table (void)
		{
			programs.clear();
		}
		virtual ~table (void)
		{
			programs.clear();
		}

		ST_SECTION_HEADER header;
		std::vector <program> programs;
	};

public:
	program_association_table (void);
	explicit program_association_table (int fifo_num);
	virtual ~program_association_table (void);


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
	void release_table (table* p_erase);

	bool refresh_sub_tables_by_version_number (table* p_new_table);
	void refresh_sub_tables (table* p_new_table);


	std::vector <table*> tables_;
};


} // namespace parser

#endif
