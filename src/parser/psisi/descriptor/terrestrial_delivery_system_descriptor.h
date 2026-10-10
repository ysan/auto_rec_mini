#ifndef _TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR_H_
#define _TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "defs.h"
#include "ts_arib_common.h"
#include "descriptor.h"

namespace parser {



class terrestrial_delivery_system_descriptor : public descriptor
{
public:
	class freq {
	public:
		freq (void)
			:frequency (0)
		{}
		virtual ~freq (void) {}

		uint16_t frequency;
	};
public:
	explicit terrestrial_delivery_system_descriptor (const descriptor &obj);
	virtual ~terrestrial_delivery_system_descriptor (void);

	void dump (void) const override;

	uint16_t area_code;
	uint8_t guard_interval;
	uint8_t transmission_mode;
	std::vector <freq> freqs;

private:
	bool parse (void);

};


} // namespace parser

#endif
