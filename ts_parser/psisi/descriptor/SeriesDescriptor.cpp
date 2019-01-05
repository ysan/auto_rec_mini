#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "SeriesDescriptor.h"
#include "Utils.h"
#include "aribstr.h"


CSeriesDescriptor::CSeriesDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
	,series_id (0)
	,repeat_label (0)
	,program_pattern (0)
	,expire_date_valid_flag (0)
	,expire_date (0)
	,episode_number (0)
	,last_episode_number (0)
	,m_series_name_char_len (0)
{
	if (!isValid) {
		return;
	}

	memset (series_name_char, 0x00, sizeof(series_name_char));

	if (!parse()) {
		isValid = false;
	}
}

CSeriesDescriptor::~CSeriesDescriptor (void)
{
}

bool CSeriesDescriptor::parse (void)
{
	memset (series_name_char, 0x00, sizeof(series_name_char));

	uint8_t *p = data;

	series_id = *p << 8 | *(p+1);
	p += 2;
	repeat_label = (*p >> 4) & 0x0f;
	program_pattern = (*p >> 1) & 0x07;
	expire_date_valid_flag = *p & 0x01;
	p += 1;
	expire_date = *p << 8 | *(p+1);
	p += 2;
	episode_number = *p << 4 | ((*(p+1) >> 4) & 0x0f);
	last_episode_number = (*(p+1) & 0x0f) << 8 | *(p+2);
	p += 3;

	m_series_name_char_len = length - (p - data);
	memcpy (series_name_char, p, m_series_name_char_len);
	p += m_series_name_char_len;

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CSeriesDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	char aribstr [MAXSECLEN];

	CDescriptor::dump (true);

	_UTL_LOG_I ("series_id              [0x%04x]\n", series_id);
	_UTL_LOG_I ("repeat_label           [0x%02x]\n", repeat_label);
	_UTL_LOG_I ("program_pattern        [0x%02x]\n", program_pattern);
	_UTL_LOG_I ("expire_date_valid_flag [0x%02x]\n", expire_date_valid_flag);
	_UTL_LOG_I ("expire_date            [0x%04x]\n", expire_date);
	_UTL_LOG_I ("episode_number         [0x%04x]\n", episode_number);
	_UTL_LOG_I ("last_episode_number    [0x%04x]\n", last_episode_number);

	memset (aribstr, 0x00, MAXSECLEN);
	AribToString (aribstr, (const char*)series_name_char, m_series_name_char_len);
	_UTL_LOG_I ("series_name_char       [%s]\n", aribstr);
}
