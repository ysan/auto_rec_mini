#ifndef _NETWORK_NAME_DESCRIPTOR_H_
#define _NETWORK_NAME_DESCRIPTOR_H_

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



class network_name_descriptor : public descriptor
{
public:
	explicit network_name_descriptor (const descriptor &obj);
	virtual ~network_name_descriptor (void);

	void dump (void) const override;

	uint8_t name_char [0xff];

private:
	bool parse (void);

};


} // namespace parser

#endif
