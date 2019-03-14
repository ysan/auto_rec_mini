#ifndef _SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR_H_
#define _SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CSatelliteDeliverySystemDescriptor : public CDescriptor
{
public:
	explicit CSatelliteDeliverySystemDescriptor (const CDescriptor &obj);
	virtual ~CSatelliteDeliverySystemDescriptor (void);

	void dump (void) const override;

	uint32_t frequency;
	uint16_t orbital_position;
	uint8_t west_east_flag;
	uint8_t polarisation;
	uint8_t modulation; 
	uint32_t symbol_rate; 
	uint8_t FEC_inner; 

private:
	bool parse (void);

};

#endif
