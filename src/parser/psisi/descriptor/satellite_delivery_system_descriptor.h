#ifndef _SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR_H_
#define _SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"
#include "descriptor.h"

namespace parser {



class satellite_delivery_system_descriptor : public descriptor
{
public:
	explicit satellite_delivery_system_descriptor (const descriptor &obj);
	virtual ~satellite_delivery_system_descriptor (void);

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


} // namespace parser

#endif
