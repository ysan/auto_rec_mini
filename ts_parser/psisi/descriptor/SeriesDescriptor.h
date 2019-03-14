#ifndef _SERIES_DESCRIPTOR_H_
#define _SERIES_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Defs.h"
#include "TsAribCommon.h"
#include "Descriptor.h"


class CSeriesDescriptor : public CDescriptor
{
public:
	explicit CSeriesDescriptor (const CDescriptor &obj);
	virtual ~CSeriesDescriptor (void);

	void dump (void) const override;

	uint16_t series_id;
	uint8_t repeat_label;
	uint8_t program_pattern;
	uint8_t expire_date_valid_flag;
	uint16_t expire_date;
	uint16_t episode_number;
	uint16_t last_episode_number;
	uint8_t series_name_char [0xff];

private:
	bool parse (void);

	int m_series_name_char_len;
};

#endif
