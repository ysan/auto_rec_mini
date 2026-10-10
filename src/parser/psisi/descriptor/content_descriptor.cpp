#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "content_descriptor.h"
#include "utils.h"
#include "aribstr.h"

namespace parser {



content_descriptor::content_descriptor (const descriptor &obj)
	:descriptor (obj)
{
	if (!is_valid) {
		return;
	}

	contents.clear();

	if (!parse()) {
		is_valid = false;
	}
}

content_descriptor::~content_descriptor (void)
{
	contents.clear();
}

bool content_descriptor::parse (void)
{
	uint8_t *p = data;

	int content_len = length;
	while (content_len > 0) {
		content con;

		con.content_nibble_level_1 = (*p >> 4) & 0x0f;
		con.content_nibble_level_2 = *p & 0x0f;
		p += 1;
		con.user_nibble_1 = (*p >> 4) & 0x0f;
		con.user_nibble_2 = *p & 0x0f;
		p += 1;

		content_len -= 2 ;
		if (content_len < 0) {
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

void content_descriptor::dump (void) const
{
	_UTL_LOG_I ("%s\n", __PRETTY_FUNCTION__);

	descriptor::dump (true);

	std::vector<content>::const_iterator iter_con = contents.begin();
	for (; iter_con != contents.end(); ++ iter_con) {
		_UTL_LOG_I ("\n--  content  --\n");
		_UTL_LOG_I (
			"content_nibble_level_1 [0x%02x][%s]\n",
			iter_con->content_nibble_level_1,
			ts_arib_common::getGenre_lvl1(iter_con->content_nibble_level_1)
		);
		_UTL_LOG_I (
			"content_nibble_level_2 [0x%02x][%s]\n",
			iter_con->content_nibble_level_2,
			ts_arib_common::getGenre_lvl2((iter_con->content_nibble_level_1 << 4 | iter_con->content_nibble_level_2) & 0xff)
		);
		_UTL_LOG_I ("user_nibble_1          [0x%02x]\n", iter_con->user_nibble_1);
		_UTL_LOG_I ("user_nibble_2          [0x%02x]\n", iter_con->user_nibble_2);
	}
}


} // namespace parser
