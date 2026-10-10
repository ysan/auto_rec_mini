#ifndef _COMPONENT_DESCRIPTOR_H_
#define _COMPONENT_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"
#include "descriptor.h"

namespace parser {



class component_descriptor : public descriptor
{
public:
	explicit component_descriptor (const descriptor &obj);
	virtual ~component_descriptor (void);

	void dump (void) const override;

	uint8_t reserved_future_use;
	uint8_t stream_content;
	uint8_t component_type; 
	uint8_t component_tag; 
	uint8_t ISO_639_language_code [4];
	uint8_t component_text [0xff];

private:
	bool parse (void);

};


} // namespace parser

#endif
