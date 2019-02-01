#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ContentDescriptor.h"
#include "Utils.h"
#include "aribstr.h"


CContentDescriptor::CContentDescriptor (const CDescriptor &obj)
	:CDescriptor (obj)
{
	if (!isValid) {
		return;
	}

	contents.clear();

	if (!parse()) {
		isValid = false;
	}
}

CContentDescriptor::~CContentDescriptor (void)
{
	contents.clear();
}

bool CContentDescriptor::parse (void)
{
	uint8_t *p = data;

	int contentLen = length;
	while (contentLen > 0) {
		CContent con;

		con.content_nibble_level_1 = (*p >> 4) & 0x0f;
		con.content_nibble_level_2 = *p & 0x0f;
		p += 1;
		con.user_nibble_1 = (*p >> 4) & 0x0f;
		con.user_nibble_2 = *p & 0x0f;
		p += 1;

		contentLen -= 2 ;
		if (contentLen < 0) {
			_UTL_LOG_W ("invalid ContentDescriptor content");
			return false;
		}

		contents.push_back (con);
	}

	// length check
	if (length != (p - data)) {
		return false;
	}

	return true;
}

void CContentDescriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	CDescriptor::dump (true);

	std::vector<CContent>::const_iterator iter_con = contents.begin();
	for (; iter_con != contents.end(); ++ iter_con) {
		_UTL_LOG_I ("\n--  content  --\n");
		_UTL_LOG_I (
			"content_nibble_level_1 [0x%02x][%s]\n",
			iter_con->content_nibble_level_1,
			CTsCommon::getGenre_lvl1(iter_con->content_nibble_level_1)
		);
		_UTL_LOG_I (
			"content_nibble_level_2 [0x%02x][%s]\n",
			iter_con->content_nibble_level_2,
			CTsCommon::getGenre_lvl2((iter_con->content_nibble_level_1 << 4 | iter_con->content_nibble_level_2) & 0xff)
		);
		_UTL_LOG_I ("user_nibble_1          [0x%02x]\n", iter_con->user_nibble_1);
		_UTL_LOG_I ("user_nibble_2          [0x%02x]\n", iter_con->user_nibble_2);
	}
}
