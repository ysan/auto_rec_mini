#ifndef _TIME_OFFSET_TABLE_H_
#define _TIME_OFFSET_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"
#include "section_parser.h"

namespace parser {



class time_offset_table : public section_parser
{
public:
	explicit time_offset_table (uint8_t fifo_num);
	virtual ~time_offset_table (void);


	// section_parser
	void on_section_completed (const section_info *p_comp_section) override;

	void dump (void) const;
	void dump (const section_info *p_sect_info) const;

private:

};


} // namespace parser

#endif
