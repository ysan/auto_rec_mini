#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TSInformationDescriptor.h"
#include "Utils.h"
#include "aribstr.h"


CTSInformationDescriptor::CTSInformationDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
	,remote_control_key_id (0)
	,length_of_ts_name (0)
	,transmission_type_count (0)
{
	if (!isValid) {
		return;
	}

	memset (ts_name_char, 0x00, sizeof(ts_name_char));
	transmissions.clear();

	if (!parse()) {
		isValid = false;
	}
}

CTSInformationDescriptor::~CTSInformationDescriptor (void)
{
}

bool CTSInformationDescriptor::parse (void)
{
	uint8_t *p = data;

	remote_control_key_id = *p;
	p += 1;
	length_of_ts_name = (*p >> 2) & 0x3f;
	transmission_type_count = *p & 0x03;
	p += 1;

	memcpy (ts_name_char, p, length_of_ts_name);
	p += length_of_ts_name;

	int transmissionCnt = transmission_type_count;
	while (transmissionCnt > 0) {

		CTransmission tm ;

		tm.transmission_type_info = *p;
		p += 1;
		tm.num_of_service = *p;
		p += 1;

		int serviceCnt = tm.num_of_service;
		while (serviceCnt > 0) {

			CTransmission::CService svc;

			svc.service_id = *p << 8 | *(p+1);
			p += 2;

			-- serviceCnt;
			tm.services.push_back (svc);
		}

		-- transmissionCnt ;
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

void CTSInformationDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	CDescriptor::dump (true);

	_UTL_LOG_I ("remote_control_key_id   [0x%02x]\n", remote_control_key_id);
	_UTL_LOG_I ("length_of_ts_name       [0x%02x]\n", length_of_ts_name);
	_UTL_LOG_I ("transmission_type_count [0x%02x]\n", transmission_type_count);

	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)ts_name_char, (int)length_of_ts_name);
	_UTL_LOG_I ("ts_name_char            [%s]\n", aribstr);

	std::vector<CTransmission>::const_iterator iter_trans = transmissions.begin();
	for (; iter_trans != transmissions.end(); ++ iter_trans) {
		_UTL_LOG_I ("\n--  transmission  --\n");
		_UTL_LOG_I ("transmission_type_info [0x%02x]\n", iter_trans->transmission_type_info);
		_UTL_LOG_I ("num_of_service         [0x%02x]\n", iter_trans->num_of_service);

		std::vector<CTransmission::CService>::const_iterator iter_svc = iter_trans->services.begin();
		for (; iter_svc != iter_trans->services.end(); ++ iter_svc) {
			_UTL_LOG_I ("\n--  service  --\n");
			_UTL_LOG_I ("service_id [0x%04x]\n", iter_svc->service_id);
		}
	}
}
