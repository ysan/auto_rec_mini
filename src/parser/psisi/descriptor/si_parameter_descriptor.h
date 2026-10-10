#ifndef _SI_PARAMETER_DESCRIPTOR_H_
#define _SI_PARAMETER_DESCRIPTOR_H_

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



class si_parameter_descriptor : public descriptor
{
public:
	class table {
	public:
		table (void)
			:table_id (0)
			,table_description_length (0)
		{
			memset (table_description_byte, 0x00, sizeof (table_description_byte));
		};
		virtual ~table (void) {};

		uint8_t table_id;
		uint8_t table_description_length;
		uint8_t table_description_byte [0xff];
	};

public:
	explicit si_parameter_descriptor (const descriptor &obj);
	virtual ~si_parameter_descriptor (void);

	void dump (void) const override;

	uint8_t parameter_version;
	uint16_t update_time;
	std::vector <table> tables;

private:
	bool parse (void);

};


} // namespace parser

#endif
