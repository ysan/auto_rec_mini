#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "AudioComponentDescriptor.h"
#include "Utils.h"
#include "aribstr.h"


CAudioComponentDescriptor::CAudioComponentDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
	,reserved_future_use (0)
	,stream_content (0)
	,component_type (0)
	,component_tag (0)
	,stream_type (0)
	,simulcast_group_tag (0)
	,ES_multi_lingual_flag (0)
	,main_component_flag (0)
	,quality_indicator (0)
	,sampling_rate (0)
	,reserved_future_use2 (0)
	,m_text_char_len (0)
{
	if (!isValid) {
		return;
	}

	memset (ISO_639_language_code, 0x00, sizeof(ISO_639_language_code));
	memset (ISO_639_language_code_2, 0x00, sizeof(ISO_639_language_code));
	memset (text_char, 0x00, sizeof(text_char));

	if (!parse()) {
		isValid = false;
	}
}

CAudioComponentDescriptor::~CAudioComponentDescriptor (void)
{
}

bool CAudioComponentDescriptor::parse (void)
{
	memset (ISO_639_language_code, 0x00, sizeof(ISO_639_language_code));
	memset (ISO_639_language_code_2, 0x00, sizeof(ISO_639_language_code_2));
	memset (text_char, 0x00, sizeof(text_char));

	uint8_t *p = data;

	reserved_future_use =  (*p >> 4) & 0x0f;
	stream_content = *p & 0x0f;
	p += 1;
	component_type = *p;
	p += 1;
	component_tag = *p;
	p += 1;
	stream_type = *p;
	p += 1;
	simulcast_group_tag = *p;
	p += 1;
	ES_multi_lingual_flag = (*p >> 7) & 0x01;
	main_component_flag = (*p >> 6) & 0x01;
	quality_indicator = (*p >> 5) & 0x03;
	sampling_rate = *p & 0x07;
	reserved_future_use2 = *p & 0x01;
	p += 1;

	memcpy (ISO_639_language_code, p, 3);
	p += 3 ;

	if (ES_multi_lingual_flag) {
		memcpy (ISO_639_language_code_2, p, 3);
		p += 3 ;
	}

	m_text_char_len = length - (p - data);
	memcpy (text_char, p, m_text_char_len);
	p += m_text_char_len;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CAudioComponentDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	CDescriptor::dump (true);

	_UTL_LOG_I ("stream_content              [0x%02x]\n", stream_content);
	_UTL_LOG_I ("component_type              [0x%02x][%s]\n",
		component_type, CTsAribCommon::getAudioComponentType(component_type));
	_UTL_LOG_I ("component_tag               [0x%02x]\n", component_tag);
	_UTL_LOG_I ("stream_type                 [0x%02x]\n", stream_type);
	_UTL_LOG_I ("simulcast_group_tag         [0x%02x]\n", simulcast_group_tag);
	_UTL_LOG_I ("ES_multi_lingual_flag       [0x%02x][%s]\n",
		ES_multi_lingual_flag, ES_multi_lingual_flag ? "二ヶ国語" : "-");
	_UTL_LOG_I ("main_component_flag         [0x%02x][%s]\n",
		main_component_flag, main_component_flag ? "主" : "副");
	_UTL_LOG_I ("quality_indicator           [0x%02x][%s]\n",
		quality_indicator, CTsAribCommon::getAudioQuality(quality_indicator));
	_UTL_LOG_I ("sampling_rate               [0x%02x][%s]\n",
		sampling_rate, CTsAribCommon::getAudioSamplingRate(sampling_rate));

	_UTL_LOG_I ("ISO_639_language_code       [%s]\n", (char*)ISO_639_language_code);

	if (ES_multi_lingual_flag) {
		_UTL_LOG_I ("ISO_639_language_code_2 [%s]\n", (char*)ISO_639_language_code_2);
	}

	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)text_char, m_text_char_len);
	_UTL_LOG_I ("text_char                   [%s]\n", aribstr);
}
