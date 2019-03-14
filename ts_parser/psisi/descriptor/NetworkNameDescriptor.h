#ifndef _NETWORK_NAME_DESCRIPTOR_H_
#define _NETWORK_NAME_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CNetworkNameDescriptor : public CDescriptor
{
public:
	explicit CNetworkNameDescriptor (const CDescriptor &obj);
	virtual ~CNetworkNameDescriptor (void);

	void dump (void) const override;

	uint8_t name_char [0xff];

private:
	bool parse (void);

};

#endif
