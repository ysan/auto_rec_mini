#ifndef _PARTIAL_RECEPTION_DESCRIPTOR_H_
#define _PARTIAL_RECEPTION_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsCommon.h"
#include "Descriptor.h"


class CPartialReceptionDescriptor : public CDescriptor
{
public:
	class CService {
	public:
		CService (void)
			:service_id (0)
		{}
		virtual ~CService (void) {}

		uint16_t service_id;
	};
public:
	explicit CPartialReceptionDescriptor (const CDescriptor &obj);
	virtual ~CPartialReceptionDescriptor (void);

	void dump (void) const override;

	std::vector <CService> services;

private:
	bool parse (void);

};

#endif
