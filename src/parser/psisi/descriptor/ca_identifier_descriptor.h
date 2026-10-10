#ifndef _CA_IDENTIFIER_DESCRIPTOR_H_
#define _CA_IDENTIFIER_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "defs.h"
#include "ts_arib_common.h"
#include "descriptor.h"

namespace parser {



class ca_identifier_descriptor : public descriptor
{
public:
	class ca_system {
	public:
		ca_system (void)
			:CA_system_id (0)
		{};
		virtual ~ca_system (void) {};

		uint16_t CA_system_id;
	};

public:
	explicit ca_identifier_descriptor (const descriptor &obj);
	virtual ~ca_identifier_descriptor (void);

	void dump (void) const override;

	std::vector <ca_system> ca_systems;

private:
	bool parse (void);

};


} // namespace parser

#endif
