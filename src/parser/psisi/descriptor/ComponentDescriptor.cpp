#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ComponentDescriptor.h"
#include "Utils.h"
#include "aribstr.h"


CComponentDescriptor::CComponentDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
	,reserved_future_use (0)
	,stream_content (0)
	,component_type (0)
	,component_tag (0)
{
	if (!isValid) {
		return;
	}

	memset (ISO_639_language_code, 0x00, sizeof(ISO_639_language_code));
	memset (component_text, 0x00, sizeof(component_text));

	if (!parse()) {
		isValid = false;
	}
}

CComponentDescriptor::~CComponentDescriptor (void)
{
}

bool CComponentDescriptor::parse (void)
{
	memset (ISO_639_language_code, 0x00, sizeof(ISO_639_language_code));
	memset (component_text, 0x00, sizeof(component_text));

	uint8_t *p = data;

	reserved_future_use = (*p >> 4) & 0x0f;
	stream_content = *p & 0x0f;
	p += 1;
	component_type = *p;
	p += 1;
	component_tag = *p;
	p += 1;

	memcpy (ISO_639_language_code, p, 3);
	p += 3 ;

	memcpy (component_text, p, length - 6);
	p += length - 6;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CComponentDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	CDescriptor::dump (true);

	_UTL_LOG_I ("stream_content        [0x%02x]\n", stream_content);
	_UTL_LOG_I ("component_type        [0x%02x][%s][%s]\n",
		component_type, CTsAribCommon::getVideoComponentType(component_type), CTsAribCommon::getVideoRatio(component_type));
	_UTL_LOG_I ("component_tag         [0x%02x]\n", component_tag);

	_UTL_LOG_I ("ISO_639_language_code [%s]\n", ISO_639_language_code);

	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)component_text, (int)(length - 6));
	_UTL_LOG_I ("component_text        [%s]\n", aribstr);
}
