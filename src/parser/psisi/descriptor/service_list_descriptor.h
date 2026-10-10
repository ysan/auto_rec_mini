#ifndef _SERVICE_LIST_DESCRIPTOR_H_
#define _SERVICE_LIST_DESCRIPTOR_H_

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



class service_list_descriptor : public descriptor
{
public:
	class service {
	public:
		service (void)
			:service_id (0)
			,service_type (0)
		{}
		virtual ~service (void) {}

		uint16_t service_id;
		uint8_t service_type;
	};
public:
	explicit service_list_descriptor (const descriptor &obj);
	virtual ~service_list_descriptor (void);

	void dump (void) const override;

	std::vector <service> services;

private:
	bool parse (void);

};


} // namespace parser

#endif
