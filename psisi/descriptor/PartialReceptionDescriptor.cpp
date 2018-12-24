#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "PartialReceptionDescriptor.h"
#include "Utils.h"


CPartialReceptionDescriptor::CPartialReceptionDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
{
	if (!isValid) {
		return;
	}

	services.clear();

	if (!parse()) {
		isValid = false;
	}
}

CPartialReceptionDescriptor::~CPartialReceptionDescriptor (void)
{
}

bool CPartialReceptionDescriptor::parse (void)
{
	uint8_t *p = data;

	int serviceLen = length;
	while (serviceLen > 0) {

		CService svc;
		svc.service_id = *p << 8 | *(p+1);
		p += 2;

		serviceLen -= 2 ;
		if (serviceLen < 0) {
			_UTL_LOG_W ("invalid PartialReceptionDescriptor service");
			return false;
		}

		services.push_back (svc);
	}

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CPartialReceptionDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	CDescriptor::dump (true);

	std::vector<CService>::const_iterator iter_svc = services.begin();
	for (; iter_svc != services.end(); ++ iter_svc) {
		_UTL_LOG_I ("\n--  service  --\n");
		_UTL_LOG_I ("service_id [0x%04x]\n", iter_svc->service_id);
	}
}
