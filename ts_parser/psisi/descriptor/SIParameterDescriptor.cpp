#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "SIParameterDescriptor.h"
#include "Utils.h"


CSIParameterDescriptor::CSIParameterDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
	,parameter_version (0)
	,update_time (0)
{
	if (!isValid) {
		return;
	}

	tables.clear();

	if (!parse()) {
		isValid = false;
	}
}

CSIParameterDescriptor::~CSIParameterDescriptor (void)
{
	tables.clear();
}

bool CSIParameterDescriptor::parse (void)
{
	uint8_t *p = data;

	parameter_version = *p;
	p += 1;
	update_time = *p << 8 | *(p+1);
	p += 2;

	int tableLen = length - (p - data);
	while (tableLen > 0) {

		CTable tbl;

		tbl.table_id = *p;
		p += 1;
		tbl.table_description_length = *p;
		p += 1;

		memcpy (tbl.table_description_byte, p, tbl.table_description_length);
		p += tbl.table_description_length;

		tableLen -= (2 + tbl.table_description_length);
		if (tableLen < 0) {
			_UTL_LOG_W ("invalid SIParameterDescriptor table");
			return false;
		}

		tables.push_back (tbl);
	}

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CSIParameterDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	CDescriptor::dump (true);

	_UTL_LOG_I ("parameter_version            [0x%02x]\n", parameter_version);
	_UTL_LOG_I ("update_time                  [0x%04x]\n", update_time);

	std::vector<CTable>::const_iterator iter_tbl = tables.begin();
	for (; iter_tbl != tables.end(); ++ iter_tbl) {
		_UTL_LOG_I ("\n--  table  --\n");
		_UTL_LOG_I ("table_id                 [0x%02x]\n", iter_tbl->table_id);
		_UTL_LOG_I ("table_description_length [%d]\n", iter_tbl->table_description_length);
		_UTL_LOG_I ("table_description_byte   [");
		for (int i = 0; i < iter_tbl->table_description_length; ++ i) {
			_UTL_LOG_I ("0x%02x,", iter_tbl->table_description_byte [i]);
		}
		_UTL_LOG_I ("]\n");
	}
}
