#ifndef _SHORT_EVENT_DESCRIPTOR_H_
#define _SHORT_EVENT_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CShortEventDescriptor : public CDescriptor
{
public:
	explicit CShortEventDescriptor (const CDescriptor &obj);
	virtual ~CShortEventDescriptor (void);

	void dump (void) const override;

	uint8_t ISO_639_language_code [4];
	uint8_t event_name_length;
	uint8_t event_name_char [0xff];
	uint8_t text_length; 
	uint8_t text_char [0xff];

private:
	bool parse (void);

};

#endif
