#ifndef _EXTENDED_EVENT_DESCRIPTOR_H_
#define _EXTENDED_EVENT_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CExtendedEventDescriptor : public CDescriptor
{
public:
	class CItem {
	public:
		CItem (void)
			:item_description_length (0)
			,item_length (0)
		{
			memset (item_description_char, 0x00, sizeof(item_description_char));
			memset (item_char, 0x00, sizeof(item_char));
		}
		virtual ~CItem (void) {}

		uint8_t item_description_length;
		uint8_t item_description_char [0xff];
		uint8_t item_length;
		uint8_t item_char [0xff];
	};

public:
	explicit CExtendedEventDescriptor (const CDescriptor &obj);
	virtual ~CExtendedEventDescriptor (void);

	void dump (void) const override;

	uint8_t descriptor_number;
	uint8_t last_descriptor_number;
	uint8_t ISO_639_language_code [4];
	uint8_t length_of_items;
	std::vector <CItem> items;
	uint8_t text_length;
	uint8_t text_char [0xff];

private:
	bool parse (void);

};

#endif
