#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "service_descriptor.h"
#include "utils.h"
#include "aribstr.h"

namespace parser {



service_descriptor::service_descriptor (const descriptor &obj)
	:descriptor (obj)
	,service_type (0)
	,service_provider_name_length (0)
	,service_name_length (0)
{
	if (!is_valid) {
		return;
	}

	memset (service_provider_name_char, 0x00, sizeof(service_provider_name_char));
	memset (service_name_char, 0x00, sizeof(service_name_char));

	if (!parse()) {
		is_valid = false;
	}
}

service_descriptor::~service_descriptor (void)
{
}

bool service_descriptor::parse (void)
{
	memset (service_provider_name_char, 0x00, sizeof(service_provider_name_char));
	memset (service_name_char, 0x00, sizeof(service_name_char));

	uint8_t *p = data;

	service_type = *p;
	p += 1;
	service_provider_name_length = *p;
	p += 1;
	memcpy (service_provider_name_char, p, service_provider_name_length);
	p += service_provider_name_length;
	service_name_length = *p;
	p += 1;
	memcpy (service_name_char, p, service_name_length);
	p += service_name_length;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void service_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	descriptor::dump (true);

	_UTL_LOG_I ("service_type                 [0x%02x]\n", service_type);

	_UTL_LOG_I ("service_provider_name_length [%d]\n", service_provider_name_length);
	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)service_provider_name_char, (int)service_provider_name_length);
	_UTL_LOG_I ("service_provider_name_char   [%s]\n", aribstr);

	_UTL_LOG_I ("service_name_length          [%d]\n", service_name_length);
	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)service_name_char, (int)service_name_length);
	_UTL_LOG_I ("service_name_char            [%s]\n", aribstr);
}


} // namespace parser
