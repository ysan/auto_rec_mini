#ifndef _SI_PARAMETER_DESCRIPTOR_H_
#define _SI_PARAMETER_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CSIParameterDescriptor : public CDescriptor
{
public:
	class CTable {
	public:
		CTable (void)
			:table_id (0)
			,table_description_length (0)
		{
			memset (table_description_byte, 0x00, sizeof (table_description_byte));
		};
		virtual ~CTable (void) {};

		uint8_t table_id;
		uint8_t table_description_length;
		uint8_t table_description_byte [0xff];
	};

public:
	explicit CSIParameterDescriptor (const CDescriptor &obj);
	virtual ~CSIParameterDescriptor (void);

	void dump (void) const override;

	uint8_t parameter_version;
	uint16_t update_time;
	std::vector <CTable> tables;

private:
	bool parse (void);

};

#endif
