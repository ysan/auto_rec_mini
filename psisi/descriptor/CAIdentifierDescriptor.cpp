#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "CAIdentifierDescriptor.h"
#include "Utils.h"


CCAIdentifierDescriptor::CCAIdentifierDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
{
	if (!isValid) {
		return;
	}

	ca_systems.clear();

	if (!parse()) {
		isValid = false;
	}
}

CCAIdentifierDescriptor::~CCAIdentifierDescriptor (void)
{
}

bool CCAIdentifierDescriptor::parse (void)
{
	uint8_t *p = data;

	int casLen = length;
	while (casLen > 0) {

		CCASystem cas ;

		cas.CA_system_id = *p << 8 | *(p+1);
		p += 2;

		casLen -= 2;
		if (casLen < 0) {
			_UTL_LOG_W ("invalid CAIdentifierDescriptor ca_system");
			return false;
		}

		ca_systems.push_back (cas);
	}

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CCAIdentifierDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	CDescriptor::dump (true);

	std::vector<CCASystem>::const_iterator iter_cas = ca_systems.begin();
	for (; iter_cas != ca_systems.end(); ++ iter_cas) {
		_UTL_LOG_I ("\n--  ca_system  --\n");
		_UTL_LOG_I ("CA_system_id [0x%04x]\n", iter_cas->CA_system_id);
	}
}
