#ifndef _CONDITIONAL_ACCESS_TABLE_H_
#define _CONDITIONAL_ACCESS_TABLE_H_

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



class conditional_access_table : public section_parser
{
public:
	class table {
	public:
		table (void)
		{
			descriptors.clear();
		}
		virtual ~table (void)
		{
			descriptors.clear();
		}

		ST_SECTION_HEADER header;
		std::vector <descriptor> descriptors;
	};

public:
	conditional_access_table (void);
	explicit conditional_access_table (int fifo_num);
	virtual ~conditional_access_table (void);


	// section_parser
	void on_section_completed (const section_info *p_comp_section) override;

	void dump_tables (void);
	void dump_table (const table* p_table) const;
	void clear (void);

	std::vector <table*> * get_tables (void) {
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
