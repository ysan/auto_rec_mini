#ifndef _SERVICE_LIST_DESCRIPTOR_H_
#define _SERVICE_LIST_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CServiceListDescriptor : public CDescriptor
{
public:
	class CService {
	public:
		CService (void)
			:service_id (0)
			,service_type (0)
		{}
		virtual ~CService (void) {}

		uint16_t service_id;
		uint8_t service_type;
	};
public:
	explicit CServiceListDescriptor (const CDescriptor &obj);
	virtual ~CServiceListDescriptor (void);

	void dump (void) const override;

	std::vector <CService> services;

private:
	bool parse (void);

};

#endif
