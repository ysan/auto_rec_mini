#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "StreamIdentifierDescriptor.h"
#include "Utils.h"


CStreamIdentifierDescriptor::CStreamIdentifierDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
	,component_tag (0)
{
	if (!isValid) {
		return;
	}

	if (!parse()) {
		isValid = false;
	}
}

CStreamIdentifierDescriptor::~CStreamIdentifierDescriptor (void)
{
}

bool CStreamIdentifierDescriptor::parse (void)
{
	uint8_t *p = data;

	component_tag = *p;
	p += 1;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CStreamIdentifierDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	CDescriptor::dump (true);

	_UTL_LOG_I ("component_tag [0x%02x]\n", component_tag);
}
