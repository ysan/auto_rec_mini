#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TerrestrialDeliverySystemDescriptor.h"
#include "Utils.h"


CTerrestrialDeliverySystemDescriptor::CTerrestrialDeliverySystemDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
	,area_code (0)
	,guard_interval (0)
	,transmission_mode (0)
{
	if (!isValid) {
		return;
	}

	freqs.clear();

	if (!parse()) {
		isValid = false;
	}
}

CTerrestrialDeliverySystemDescriptor::~CTerrestrialDeliverySystemDescriptor (void)
{
	freqs.clear();
}

bool CTerrestrialDeliverySystemDescriptor::parse (void)
{
	uint8_t *p = data;

	memcpy (&area_code, p, 2);
	area_code = (area_code >> 4) & 0x0fff;
	guard_interval = ((*p+1) >> 2) & 0x03;
	transmission_mode = *(p+1) & 0x03;
	p += 2;

	int freqLen = length - (p - data);
	while (freqLen > 0) {

		CFreq f ;
		f.frequency = *p << 8 | *(p+1);
		p += 2;

		freqLen -= 2 ;
		if (freqLen < 0) {
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

void CTerrestrialDeliverySystemDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	CDescriptor::dump (true);

	_UTL_LOG_I ("area_code         [0x%04x]\n", area_code);
	_UTL_LOG_I ("guard_interval    [0x%02x]\n", guard_interval);
	_UTL_LOG_I ("transmission_mode [0x%02x]\n", transmission_mode);

	std::vector<CFreq>::const_iterator iter_freq = freqs.begin();
	for (; iter_freq != freqs.end(); ++ iter_freq) {
		_UTL_LOG_I ("\n--  freq  --\n");
		_UTL_LOG_I ("frequency [%.3f]\n", (double)(iter_freq->frequency / 7));
	}
}
