#ifndef _CA_IDENTIFIER_DESCRIPTOR_H_
#define _CA_IDENTIFIER_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsCommon.h"
#include "Descriptor.h"


class CCAIdentifierDescriptor : public CDescriptor
{
public:
	class CCASystem {
	public:
		CCASystem (void)
			:CA_system_id (0)
		{};
		virtual ~CCASystem (void) {};

		uint16_t CA_system_id;
	};

public:
	explicit CCAIdentifierDescriptor (const CDescriptor &obj);
	virtual ~CCAIdentifierDescriptor (void);

	void dump (void) const override;

	std::vector <CCASystem> ca_systems;

private:
	bool parse (void);

};

#endif
