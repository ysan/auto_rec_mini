#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ts_information_descriptor.h"
#include "utils.h"
#include "aribstr.h"

namespace parser {



ts_information_descriptor::ts_information_descriptor (const descriptor &obj)
	:descriptor (obj)
	,remote_control_key_id (0)
	,length_of_ts_name (0)
	,transmission_type_count (0)
{
	if (!is_valid) {
		return;
	}

	memset (ts_name_char, 0x00, sizeof(ts_name_char));
	transmissions.clear();

	if (!parse()) {
		is_valid = false;
	}
}

ts_information_descriptor::~ts_information_descriptor (void)
{
	transmissions.clear();
}

bool ts_information_descriptor::parse (void)
{
	uint8_t *p = data;

	remote_control_key_id = *p;
	p += 1;
	length_of_ts_name = (*p >> 2) & 0x3f;
	transmission_type_count = *p & 0x03;
	p += 1;

	memcpy (ts_name_char, p, length_of_ts_name);
	p += length_of_ts_name;

	int transmission_cnt = transmission_type_count;
	while (transmission_cnt > 0) {

		transmission tm ;

		tm.transmission_type_info = *p;
		p += 1;
		tm.num_of_service = *p;
		p += 1;

		int service_cnt = tm.num_of_service;
		while (service_cnt > 0) {

			transmission::service svc;

			svc.service_id = *p << 8 | *(p+1);
			p += 2;

			-- service_cnt;
			tm.services.push_back (svc);
		}

		-- transmission_cnt ;
		transmissions.push_back (tm);
	}

	// length check
//	if (length != (p - data)) {
	// 以降reserve領域
	if (length < (p - data)) {
		return false;
	}

	return true;
}

void ts_information_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	descriptor::dump (true);

	_UTL_LOG_I ("remote_control_key_id   [0x%02x]\n", remote_control_key_id);
	_UTL_LOG_I ("length_of_ts_name       [0x%02x]\n", length_of_ts_name);
	_UTL_LOG_I ("transmission_type_count [0x%02x]\n", transmission_type_count);

	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)ts_name_char, (int)length_of_ts_name);
	_UTL_LOG_I ("ts_name_char            [%s]\n", aribstr);

	std::vector<transmission>::const_iterator iter_trans = transmissions.begin();
	for (; iter_trans != transmissions.end(); ++ iter_trans) {
		_UTL_LOG_I ("\n--  transmission  --\n");
		_UTL_LOG_I ("transmission_type_info [0x%02x]\n", iter_trans->transmission_type_info);
		_UTL_LOG_I ("num_of_service         [0x%02x]\n", iter_trans->num_of_service);

		std::vector<transmission::service>::const_iterator iter_svc = iter_trans->services.begin();
		for (; iter_svc != iter_trans->services.end(); ++ iter_svc) {
			_UTL_LOG_I ("\n--  service  --\n");
			_UTL_LOG_I ("service_id [0x%04x]\n", iter_svc->service_id);
		}
	}
}


} // namespace parser
