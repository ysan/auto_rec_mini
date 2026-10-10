#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "stream_identifier_descriptor.h"
#include "utils.h"

namespace parser {



stream_identifier_descriptor::stream_identifier_descriptor (const descriptor &obj)
	:descriptor (obj)
	,component_tag (0)
{
	if (!is_valid) {
		return;
	}

	if (!parse()) {
		is_valid = false;
	}
}

stream_identifier_descriptor::~stream_identifier_descriptor (void)
{
}

bool stream_identifier_descriptor::parse (void)
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

void stream_identifier_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	descriptor::dump (true);

	_UTL_LOG_I ("component_tag [0x%02x]\n", component_tag);
}


} // namespace parser
