#ifndef _SERVICE_DESCRIPTOR_H_
#define _SERVICE_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsCommon.h"
#include "Descriptor.h"


class CServiceDescriptor : public CDescriptor
{
public:
	explicit CServiceDescriptor (const CDescriptor &obj);
	virtual ~CServiceDescriptor (void);

	void dump (void) const override;

	uint8_t service_type;
	uint8_t service_provider_name_length;
	uint8_t service_provider_name_char [0xff];
	uint8_t service_name_length;
	uint8_t service_name_char [0xff];

private:
	bool parse (void);

};

#endif
