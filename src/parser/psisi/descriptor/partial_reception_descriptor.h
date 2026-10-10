#ifndef _PARTIAL_RECEPTION_DESCRIPTOR_H_
#define _PARTIAL_RECEPTION_DESCRIPTOR_H_

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



class partial_reception_descriptor : public descriptor
{
public:
	class service {
	public:
		service (void)
			:service_id (0)
		{}
		virtual ~service (void) {}

		uint16_t service_id;
	};
public:
	explicit partial_reception_descriptor (const descriptor &obj);
	virtual ~partial_reception_descriptor (void);

	void dump (void) const override;

	std::vector <service> services;

private:
	bool parse (void);

};


} // namespace parser

#endif
