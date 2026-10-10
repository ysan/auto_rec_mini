#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "partial_reception_descriptor.h"
#include "utils.h"

namespace parser {



partial_reception_descriptor::partial_reception_descriptor (const descriptor &obj)
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

partial_reception_descriptor::~partial_reception_descriptor (void)
{
	services.clear();
}

bool partial_reception_descriptor::parse (void)
{
	uint8_t *p = data;

	int service_len = length;
	while (service_len > 0) {

		service svc;
		svc.service_id = *p << 8 | *(p+1);
		p += 2;

		service_len -= 2 ;
		if (service_len < 0) {
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

void partial_reception_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	descriptor::dump (true);

	std::vector<service>::const_iterator iter_svc = services.begin();
	for (; iter_svc != services.end(); ++ iter_svc) {
		_UTL_LOG_I ("\n--  service  --\n");
		_UTL_LOG_I ("service_id [0x%04x]\n", iter_svc->service_id);
	}
}


} // namespace parser
