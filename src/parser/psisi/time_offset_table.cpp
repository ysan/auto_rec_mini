#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "time_offset_table.h"
#include "utils.h"

namespace parser {




time_offset_table::time_offset_table (uint8_t fifo_num)
	:section_parser (fifo_num)
{
}

time_offset_table::~time_offset_table (void)
{
}

void time_offset_table::on_section_completed (const section_info *p_comp_section)
{
	if (!p_comp_section) {
		return ;
	}

	dump (p_comp_section);
}

void time_offset_table::dump (void) const
{
	section_info *p_latest = get_latest_complete_section ();
	if (!p_latest) {
		return ;
	}

	dump (p_latest);
}

void time_offset_table::dump (const section_info *p_sect_info) const
{
	if (!p_sect_info) {
		return;
	}

	uint8_t *p = p_sect_info->get_data_part_addr ();


	int mjd,jth,jtm,jts,jdy,jdm,jdd,jdw,jds;
    char wds [7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

	// 修正ユリウス日と時、分、秒を求める
	mjd = (*p << 8) | *(p+1);
	jth = ((*(p+2) & 0xf0) >> 4) * 10 + (*(p+2) & 0x0f);
	jtm = ((*(p+3) & 0xf0) >> 4) * 10 + (*(p+3) & 0x0f);
	jts = ((*(p+4) & 0xf0) >> 4) * 10 + (*(p+4) & 0x0f);

	// 修正ユリウス日を西暦1年1月1日からの日数に変換する
	jdd =mjd + 678576;
	// 日数から曜日を求める
	jdw = jdd % 7;
	// 日数から年、月、日を求める
	jdd = jdd - 1 - 31 - 28 + 365;
	jds = jdd % 146097 % 36524 % 1461 % 365 + jdd % 146097 / 36524 / 4 * 365 +jdd % 146097 % 36524 % 1461 / 365 / 4 * 365;
	jdy = jdd / 146097 * 400 + jdd % 146097 / 36524 * 100 - jdd % 146097 / 36524 / 4 + jdd % 146097 % 36524 / 1461 * 4 + jdd % 146097 % 36524 % 1461 / 365 - jdd % 146097 % 36524 % 1461 / 365 / 4;
	jdm = jds / 153 * 5 + jds % 153 / 61 * 2 + jds % 153 % 61 / 31 + 3;
	jdd = jds % 153 % 61 % 31 + 1;
	if (jdm > 12) {
		jdy = jdy + 1;
		jdm = jdm - 12;
	}
	// 年、月、日、曜日、時、分、秒を表示
	printf ("%04d/%02d/%02d %s %02d:%02d:%02d\n", jdy, jdm, jdd, wds[jdw], jth, jtm, jts);
}


} // namespace parser
