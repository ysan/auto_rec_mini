#ifndef _SERVICE_DESCRIPTOR_H_
#define _SERVICE_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"
#include "descriptor.h"

namespace parser {



class service_descriptor : public descriptor
{
public:
	explicit service_descriptor (const descriptor &obj);
	virtual ~service_descriptor (void);

	void dump (void) const override;

	uint8_t service_type;
	uint8_t service_provider_name_length;
	uint8_t service_provider_name_char [0xff];
	uint8_t service_name_length;
	uint8_t service_name_char [0xff];

private:
	bool parse (void);

};


} // namespace parser

#endif
