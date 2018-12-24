#ifndef _COMPONENT_DESCRIPTOR_H_
#define _COMPONENT_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsCommon.h"
#include "Descriptor.h"


class CComponentDescriptor : public CDescriptor
{
public:
	explicit CComponentDescriptor (const CDescriptor &obj);
	virtual ~CComponentDescriptor (void);

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

#endif
