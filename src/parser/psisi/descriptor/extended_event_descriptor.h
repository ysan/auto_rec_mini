#ifndef _EXTENDED_EVENT_DESCRIPTOR_H_
#define _EXTENDED_EVENT_DESCRIPTOR_H_

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



class extended_event_descriptor : public descriptor
{
public:
	class item {
	public:
		item (void)
			:item_description_length (0)
			,item_length (0)
		{
			memset (item_description_char, 0x00, sizeof(item_description_char));
			memset (item_char, 0x00, sizeof(item_char));
		}
		virtual ~item (void) {}

		uint8_t item_description_length;
		uint8_t item_description_char [0xff];
		uint8_t item_length;
		uint8_t item_char [0xff];
	};

public:
	explicit extended_event_descriptor (const descriptor &obj);
	virtual ~extended_event_descriptor (void);

	void dump (void) const override;

	uint8_t descriptor_number;
	uint8_t last_descriptor_number;
	uint8_t ISO_639_language_code [4];
	uint8_t length_of_items;
	std::vector <item> items;
	uint8_t text_length;
	uint8_t text_char [0xff];

private:
	bool parse (void);

};


} // namespace parser

#endif
