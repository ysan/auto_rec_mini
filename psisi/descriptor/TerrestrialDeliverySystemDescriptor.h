#ifndef _TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR_H_
#define _TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsCommon.h"
#include "Descriptor.h"


class CTerrestrialDeliverySystemDescriptor : public CDescriptor
{
public:
	class CFreq {
	public:
		CFreq (void)
			:frequency (0)
		{}
		virtual ~CFreq (void) {}

		uint16_t frequency;
	};
public:
	explicit CTerrestrialDeliverySystemDescriptor (const CDescriptor &obj);
	virtual ~CTerrestrialDeliverySystemDescriptor (void);

	void dump (void) const override;

	uint16_t area_code;
	uint8_t guard_interval;
	uint8_t transmission_mode;
	std::vector <CFreq> freqs;

private:
	bool parse (void);

};

#endif
