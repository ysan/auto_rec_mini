#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "NetworkNameDescriptor.h"
#include "Utils.h"
#include "aribstr.h"


CNetworkNameDescriptor::CNetworkNameDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
{
	if (!isValid) {
		return;
	}

	memset (name_char, 0x00, sizeof(name_char));

	if (!parse()) {
		isValid = false;
	}
}

CNetworkNameDescriptor::~CNetworkNameDescriptor (void)
{
}

bool CNetworkNameDescriptor::parse (void)
{
	uint8_t *p = data;

	memcpy (name_char, p, length);
	p += length;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CNetworkNameDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	CDescriptor::dump (true);

	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)name_char, (int)length);
	_UTL_LOG_I ("name_char [%s]\n", aribstr);
}
