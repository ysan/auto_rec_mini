#ifndef _SHORT_EVENT_DESCRIPTOR_H_
#define _SHORT_EVENT_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"
#include "descriptor.h"

namespace parser {



class short_event_descriptor : public descriptor
{
public:
	explicit short_event_descriptor (const descriptor &obj);
	virtual ~short_event_descriptor (void);

	void dump (void) const override;

	uint8_t ISO_639_language_code [4];
	uint8_t event_name_length;
	uint8_t event_name_char [0xff];
	uint8_t text_length; 
	uint8_t text_char [0xff];

private:
	bool parse (void);

};


} // namespace parser

#endif
