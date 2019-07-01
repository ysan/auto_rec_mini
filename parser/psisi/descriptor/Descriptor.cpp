#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Descriptor.h"
#include "Utils.h"


CDescriptor::CDescriptor (const uint8_t *pDesc)
	:tag (0)
	,length (0)
	,isValid (true)
{
	if (!pDesc) {
		isValid = false;
		return ;
	}

	if (!parse (pDesc)) {
		isValid = false;
	}
}

CDescriptor::CDescriptor (const CDescriptor &obj)
	:tag (0)
	,length (0)
	,isValid (true)
{
	tag = obj.tag;
	length = obj.length;
	memset (data, 0x00, sizeof(data));
	memcpy (data, obj.data, length);
	isValid = obj.isValid;
}

CDescriptor::~CDescriptor (void)
{
}

bool CDescriptor::parse (const uint8_t *pDesc)
{
	if (!pDesc) {
		return false;
	}

	uint8_t len = *(pDesc + 1);
	if (len == 0) {
		return false;
	}

	tag = *(pDesc + 0);
	length = len;
	memset (data, 0x00, sizeof(data));
	memcpy (data, (pDesc + 2), length);
	return true;
}

void CDescriptor::dump (void) const
{
	dump (true);
}

void CDescriptor::dump (bool isDataDump) const
{
	_UTL_LOG_I ("tag    [0x%02x]\n", tag);
	_UTL_LOG_I ("length [%d]\n", length);
	if (isDataDump) {
		CUtils::dumper (data, length);
	}
}
