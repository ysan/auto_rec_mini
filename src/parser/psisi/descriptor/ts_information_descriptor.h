#ifndef _TS_INFORMATION_DESCRIPTOR_H_
#define _TS_INFORMATION_DESCRIPTOR_H_

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



class ts_information_descriptor : public descriptor
{
public:
	class transmission {
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
		transmission (void)
			:transmission_type_info (0)
			,num_of_service (0)
		{
			services.clear();
		}
		virtual ~transmission (void)
		{
			services.clear();
		}

		uint8_t transmission_type_info;
		uint8_t num_of_service;
		std::vector <service> services;
	};
public:
	explicit ts_information_descriptor (const descriptor &obj);
	virtual ~ts_information_descriptor (void);

	void dump (void) const override;

	uint8_t remote_control_key_id;
	uint8_t length_of_ts_name;
	uint8_t transmission_type_count;
	uint8_t ts_name_char [0xff];
	std::vector <transmission> transmissions;
	// reserve

private:
	bool parse (void);

};


} // namespace parser

#endif
