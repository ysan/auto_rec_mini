#ifndef _LOGO_TRANSMISSION_DESCRIPTOR_H_
#define _LOGO_TRANSMISSION_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CLogoTransmissionDescriptor : public CDescriptor
{
public:
	explicit CLogoTransmissionDescriptor (const CDescriptor &obj);
	virtual ~CLogoTransmissionDescriptor (void);

	void dump (void) const override;

	uint8_t logo_transmission_type;
	uint16_t logo_id;
	uint16_t logo_version;
	uint16_t download_data_id;
	uint8_t logo_char [0xff];

private:
	bool parse (void);

};

#endif
