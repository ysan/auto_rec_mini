#ifndef _CONDITIONAL_ACCCESS_DESCRIPTOR_H_
#define _CONDITIONAL_ACCCESS_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CConditionalAccessDescriptor : public CDescriptor
{
public:
	explicit CConditionalAccessDescriptor (const CDescriptor &obj);
	virtual ~CConditionalAccessDescriptor (void);

	void dump (void) const override;

	uint16_t CA_system_ID;
	uint8_t reserve;
	uint16_t CA_PID;

private:
	bool parse (void);

};

#endif
