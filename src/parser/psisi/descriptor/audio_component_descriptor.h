#ifndef _AUDIO_COMPONENT_DESCRIPTOR_H_
#define _AUDIO_COMPONENT_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"
#include "ts_arib_common.h"
#include "descriptor.h"

namespace parser {



class audio_component_descriptor : public descriptor
{
public:
	explicit audio_component_descriptor (const descriptor &obj);
	virtual ~audio_component_descriptor (void);

	void dump (void) const override;

	uint8_t reserved_future_use;
	uint8_t stream_content;
	uint8_t component_type;
	uint8_t component_tag;
	uint8_t stream_type;
	uint8_t simulcast_group_tag;
	uint8_t ES_multi_lingual_flag;
	uint8_t main_component_flag;
	uint8_t quality_indicator;
	uint8_t sampling_rate;
	uint8_t reserved_future_use2;
	uint8_t ISO_639_language_code [4];
	uint8_t ISO_639_language_code_2 [4];
	uint8_t text_char [0xff];

private:
	bool parse (void);

	int text_char_len_;
};


} // namespace parser

#endif
