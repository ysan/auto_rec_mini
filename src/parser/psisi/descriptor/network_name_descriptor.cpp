#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "network_name_descriptor.h"
#include "utils.h"
#include "aribstr.h"

namespace parser {



network_name_descriptor::network_name_descriptor (const descriptor &obj)
	:descriptor (obj)
{
	if (!is_valid) {
		return;
	}

	memset (name_char, 0x00, sizeof(name_char));

	if (!parse()) {
		is_valid = false;
	}
}

network_name_descriptor::~network_name_descriptor (void)
{
}

bool network_name_descriptor::parse (void)
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

void network_name_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	descriptor::dump (true);

	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)name_char, (int)length);
	_UTL_LOG_I ("name_char [%s]\n", aribstr);
}


} // namespace parser
