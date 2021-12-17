#ifndef _STREAM_IDENTIFIER_DESCRIPTOR_H_
#define _STREAM_IDENTIFIER_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CStreamIdentifierDescriptor : public CDescriptor
{
public:
	explicit CStreamIdentifierDescriptor (const CDescriptor &obj);
	virtual ~CStreamIdentifierDescriptor (void);

	void dump (void) const override;

	uint8_t component_tag;

private:
	bool parse (void);

};

#endif
