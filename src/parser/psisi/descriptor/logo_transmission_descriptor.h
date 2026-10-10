#ifndef _LOGO_TRANSMISSION_DESCRIPTOR_H_
#define _LOGO_TRANSMISSION_DESCRIPTOR_H_

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



class logo_transmission_descriptor : public descriptor
{
public:
	explicit logo_transmission_descriptor (const descriptor &obj);
	virtual ~logo_transmission_descriptor (void);

	void dump (void) const override;

	uint8_t logo_transmission_type;
	uint16_t logo_id;
	uint16_t logo_version;
	uint16_t download_data_id;
	uint8_t logo_char [0xff];

private:
	bool parse (void);

};


} // namespace parser

#endif
