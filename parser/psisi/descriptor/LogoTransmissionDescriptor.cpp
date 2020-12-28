#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "LogoTransmissionDescriptor.h"
#include "Utils.h"
#include "aribstr.h"


CLogoTransmissionDescriptor::CLogoTransmissionDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
{
	if (!isValid) {
		return;
	}

	memset (logo_char, 0x00, sizeof(logo_char));

	if (!parse()) {
		isValid = false;
	}
}

CLogoTransmissionDescriptor::~CLogoTransmissionDescriptor (void)
{
}

bool CLogoTransmissionDescriptor::parse (void)
{
	uint8_t *p = data;

	logo_transmission_type = *p;
	p += 1;

	switch (logo_transmission_type) {
	case 0x01:
		logo_id = (*(p+0) & 0x01) << 8 | *(p+1);
		p += 2;
		logo_version = (*(p+0) & 0x0f) << 8 | *(p+1);
		p += 2;
		download_data_id = *(p+0) << 8 | *(p+1);
		p += 2;
		break;
	case 0x02:
		logo_id = (*(p+0) & 0x01) << 8 | *(p+1);
		p += 2;
		break;
	case 0x03: {
		uint8_t logo_char_len = length -1;
		memcpy (logo_char, p, logo_char_len);
		p += logo_char_len;
		}
		break;
	default:
		_UTL_LOG_W ("unrecognized logo_transmission_type:[%0x02x]", logo_transmission_type);
		return false;
	}

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CLogoTransmissionDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	CDescriptor::dump (true);

	_UTL_LOG_I ("logo_transmission_type [0x%02x]\n", logo_transmission_type);
	switch (logo_transmission_type) {
	case 0x01:
		_UTL_LOG_I ("logo_id                [0x%04x]\n", logo_id);
		_UTL_LOG_I ("logo_version           [0x%04x]\n", logo_version);
		_UTL_LOG_I ("download_data_id       [0x%04x]\n", download_data_id);
		break;
	case 0x02:
		_UTL_LOG_I ("logo_id                [0x%04x]\n", logo_id);
		break;
	case 0x03:
		memset (aribstr, 0x00, MAXSECLEN);
		AribToString (aribstr, (const char*)logo_char, (int)length);
		_UTL_LOG_I ("logo_char              [%s]\n", aribstr);
		break;
	default:
		break;
	}

}
