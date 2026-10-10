#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "descriptor.h"
#include "utils.h"

namespace parser {



descriptor::descriptor (const uint8_t *p_desc)
	:tag (0)
	,length (0)
	,is_valid (true)
{
	if (!p_desc) {
		is_valid = false;
		return ;
	}

	if (!parse (p_desc)) {
		is_valid = false;
	}
}

descriptor::descriptor (const descriptor &obj)
	:tag (0)
	,length (0)
	,is_valid (true)
{
	tag = obj.tag;
	length = obj.length;
	memset (data, 0x00, sizeof(data));
	memcpy (data, obj.data, length);
	is_valid = obj.is_valid;
}

descriptor::~descriptor (void)
{
}

bool descriptor::parse (const uint8_t *p_desc)
{
	if (!p_desc) {
		return false;
	}

	uint8_t len = *(p_desc + 1);
	if (len == 0) {
		return false;
	}

	tag = *(p_desc + 0);
	length = len;
	memset (data, 0x00, sizeof(data));
	memcpy (data, (p_desc + 2), length);
	return true;
}

void descriptor::dump (void) const
{
	dump (true);
}

void descriptor::dump (bool is_data_dump) const
{
	_UTL_LOG_I ("tag    [0x%02x]\n", tag);
	_UTL_LOG_I ("length [%d]\n", length);
	if (is_data_dump) {
		arec::common::utils::dumper (data, length);
	}
}


} // namespace parser
