#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "short_event_descriptor.h"
#include "utils.h"
#include "aribstr.h"

namespace parser {



short_event_descriptor::short_event_descriptor (const descriptor &obj)
	:descriptor (obj)
	,event_name_length (0)
	,text_length (0)
{
	if (!is_valid) {
		return;
	}

	memset (ISO_639_language_code, 0x00, sizeof(ISO_639_language_code));
	memset (event_name_char, 0x00, sizeof(event_name_char));
	memset (text_char, 0x00, sizeof(text_char));

	if (!parse()) {
		is_valid = false;
	}
}

short_event_descriptor::~short_event_descriptor (void)
{
}

bool short_event_descriptor::parse (void)
{
	uint8_t *p = data;

	memcpy (ISO_639_language_code, p, 3);
	p += 3 ;

	event_name_length = *p;
	p += 1;
	memcpy (event_name_char, p, event_name_length);
	p += event_name_length;

	text_length = *p;
	p += 1;
	memcpy (text_char, p, text_length);
	p += text_length;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void short_event_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	descriptor::dump (true);

	_UTL_LOG_I ("ISO_639_language_code [%s]\n", ISO_639_language_code);

	_UTL_LOG_I ("event_name_length     [%d]\n", event_name_length);
	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)event_name_char, (int)event_name_length);
	_UTL_LOG_I ("event_name_char       [%s]\n", aribstr);

	_UTL_LOG_I ("text_length           [%d]\n", text_length);
	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)text_char, (int)text_length);
	_UTL_LOG_I ("text_char             [%s]\n", aribstr);
}


} // namespace parser
