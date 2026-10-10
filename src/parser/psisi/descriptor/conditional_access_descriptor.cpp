#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "conditional_access_descriptor.h"
#include "utils.h"

namespace parser {



conditional_access_descriptor::conditional_access_descriptor (const descriptor &obj)
	:descriptor (obj)
	,CA_system_ID (0)
	,reserve (0)
	,CA_PID (0)
{
	if (!is_valid) {
		return;
	}

	if (!parse()) {
		is_valid = false;
	}
}

conditional_access_descriptor::~conditional_access_descriptor (void)
{
}

bool conditional_access_descriptor::parse (void)
{
	uint8_t *p = data;

	CA_system_ID = *p << 8 | *(p+1);
	p += 2;
	reserve = (*p >> 5) & 0x07;
	CA_PID = (*p & 0x1f) << 8 | *(p+1);
	p += 2;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void conditional_access_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	descriptor::dump (true);

	_UTL_LOG_I ("CA_system_ID [0x%04x]\n", CA_system_ID);
	_UTL_LOG_I ("CA_PID       [0x%04x]\n", CA_PID);
}


} // namespace parser
