#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "satellite_delivery_system_descriptor.h"
#include "utils.h"
#include "aribstr.h"

namespace parser {



satellite_delivery_system_descriptor::satellite_delivery_system_descriptor (const descriptor &obj)
	:descriptor (obj)
	,frequency (0)
	,orbital_position (0)
	,west_east_flag (0)
	,polarisation (0)
	,modulation (0)
	,symbol_rate (0)
	,FEC_inner (0)
{
	if (!is_valid) {
		return;
	}

	if (!parse()) {
		is_valid = false;
	}
}

satellite_delivery_system_descriptor::~satellite_delivery_system_descriptor (void)
{
}

bool satellite_delivery_system_descriptor::parse (void)
{
	uint8_t *p = data;

	memcpy (&frequency, p, 4);
	p += 4;
	orbital_position = *p << 8 | *(p+1);
	p += 2;
	west_east_flag = (*p >> 7) & 0x01;
	polarisation = (*p >> 5) & 0x03;
	modulation = *p | 0x1f;
	p += 1;
	memcpy (&symbol_rate, p, 4);
	symbol_rate = (symbol_rate >> 4) & 0x0fffffff;
	FEC_inner = *(p+3) & 0x0f;
	p += 4;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void satellite_delivery_system_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	descriptor::dump (true);

	_UTL_LOG_I ("frequency        [0x%08x]\n", frequency);
	_UTL_LOG_I ("orbital_position [0x%04x]\n", orbital_position);
	_UTL_LOG_I ("west_east_flag   [0x%02x]\n", west_east_flag);
	_UTL_LOG_I ("polarisation     [0x%02x]\n", polarisation);
	_UTL_LOG_I ("modulation       [0x%02x]\n", modulation);
	_UTL_LOG_I ("symbol_rate      [0x%08x]\n", symbol_rate);
	_UTL_LOG_I ("FEC_inner        [0x%02x]\n", FEC_inner);
}


} // namespace parser
