#ifndef _STREAM_IDENTIFIER_DESCRIPTOR_H_
#define _STREAM_IDENTIFIER_DESCRIPTOR_H_

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



class stream_identifier_descriptor : public descriptor
{
public:
	explicit stream_identifier_descriptor (const descriptor &obj);
	virtual ~stream_identifier_descriptor (void);

	void dump (void) const override;

	uint8_t component_tag;

private:
	bool parse (void);

};


} // namespace parser

#endif
