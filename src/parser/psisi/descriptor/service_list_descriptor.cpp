#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "service_list_descriptor.h"
#include "utils.h"
#include "aribstr.h"

namespace parser {



service_list_descriptor::service_list_descriptor (const descriptor &obj)
	:descriptor (obj)
{
	if (!is_valid) {
		return;
	}

	services.clear();

	if (!parse()) {
		is_valid = false;
	}
}

service_list_descriptor::~service_list_descriptor (void)
{
	services.clear();
}

bool service_list_descriptor::parse (void)
{
	uint8_t *p = data;

	int service_len = length;
	while (service_len > 0) {
		service sv;

		sv.service_id = *p << 8 | *(p+1);
		p += 2;
		sv.service_type = *p;
		p += 1;

		service_len -= 3 ;
		if (service_len < 0) {
			_UTL_LOG_W ("invalid ServiceListDescriptor service");
			return false;
		}

		services.push_back (sv);
	}

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void service_list_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	descriptor::dump (true);

	std::vector<service>::const_iterator iter_sv = services.begin();
	for (; iter_sv != services.end(); ++ iter_sv) {
		_UTL_LOG_I ("\n--  service  --\n");
		_UTL_LOG_I ("service_id   [0x%04x]\n", iter_sv->service_id);
		_UTL_LOG_I ("service_type [0x%02x]\n", iter_sv->service_type);
	}
}


} // namespace parser
