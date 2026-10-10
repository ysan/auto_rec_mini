#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ca_identifier_descriptor.h"
#include "utils.h"

namespace parser {



ca_identifier_descriptor::ca_identifier_descriptor (const descriptor &obj)
	:descriptor (obj)
{
	if (!is_valid) {
		return;
	}

	ca_systems.clear();

	if (!parse()) {
		is_valid = false;
	}
}

ca_identifier_descriptor::~ca_identifier_descriptor (void)
{
}

bool ca_identifier_descriptor::parse (void)
{
	uint8_t *p = data;

	int cas_len = length;
	while (cas_len > 0) {

		ca_system cas ;

		cas.CA_system_id = *p << 8 | *(p+1);
		p += 2;

		cas_len -= 2;
		if (cas_len < 0) {
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

void ca_identifier_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	descriptor::dump (true);

	std::vector<ca_system>::const_iterator iter_cas = ca_systems.begin();
	for (; iter_cas != ca_systems.end(); ++ iter_cas) {
		_UTL_LOG_I ("\n--  ca_system  --\n");
		_UTL_LOG_I ("CA_system_id [0x%04x]\n", iter_cas->CA_system_id);
	}
}


} // namespace parser
