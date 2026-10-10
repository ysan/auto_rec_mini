#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "terrestrial_delivery_system_descriptor.h"
#include "utils.h"

namespace parser {



terrestrial_delivery_system_descriptor::terrestrial_delivery_system_descriptor (const descriptor &obj)
	:descriptor (obj)
	,area_code (0)
	,guard_interval (0)
	,transmission_mode (0)
{
	if (!is_valid) {
		return;
	}

	freqs.clear();

	if (!parse()) {
		is_valid = false;
	}
}

terrestrial_delivery_system_descriptor::~terrestrial_delivery_system_descriptor (void)
{
	freqs.clear();
}

bool terrestrial_delivery_system_descriptor::parse (void)
{
	uint8_t *p = data;

	memcpy (&area_code, p, 2);
	area_code = (area_code >> 4) & 0x0fff;
	guard_interval = ((*p+1) >> 2) & 0x03;
	transmission_mode = *(p+1) & 0x03;
	p += 2;

	int freq_len = length - (p - data);
	while (freq_len > 0) {

		freq f ;
		f.frequency = *p << 8 | *(p+1);
		p += 2;

		freq_len -= 2 ;
		if (freq_len < 0) {
			_UTL_LOG_W ("invalid TerrestrialDeliverySystemDescriptor freq");
			return false;
		}

		freqs.push_back (f);
	}

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void terrestrial_delivery_system_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	descriptor::dump (true);

	_UTL_LOG_I ("area_code         [0x%04x]\n", area_code);
	_UTL_LOG_I ("guard_interval    [0x%02x]\n", guard_interval);
	_UTL_LOG_I ("transmission_mode [0x%02x]\n", transmission_mode);

	std::vector<freq>::const_iterator iter_freq = freqs.begin();
	for (; iter_freq != freqs.end(); ++ iter_freq) {
		_UTL_LOG_I ("\n--  freq  --\n");
		_UTL_LOG_I ("frequency [%.3f]\n", (double)(iter_freq->frequency / 7));
	}
}


} // namespace parser
