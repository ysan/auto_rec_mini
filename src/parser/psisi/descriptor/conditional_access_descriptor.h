#ifndef _CONDITIONAL_ACCCESS_DESCRIPTOR_H_
#define _CONDITIONAL_ACCCESS_DESCRIPTOR_H_

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



class conditional_access_descriptor : public descriptor
{
public:
	explicit conditional_access_descriptor (const descriptor &obj);
	virtual ~conditional_access_descriptor (void);

	void dump (void) const override;

	uint16_t CA_system_ID;
	uint8_t reserve;
	uint16_t CA_PID;

private:
	bool parse (void);

};


} // namespace parser

#endif
