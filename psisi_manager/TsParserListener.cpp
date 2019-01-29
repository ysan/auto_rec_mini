#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "TsParserListener.h"


CTsParserListener::CTsParserListener (void)
	:mEIT_H (4096*20, 20)
{
}

CTsParserListener::~CTsParserListener (void)
{
}


bool CTsParserListener::onTsAvailable (TS_HEADER *p_ts_header, uint8_t *p_payload, size_t payload_size)
{
	if (!p_ts_header || !p_payload || payload_size == 0) {
		// through
		return true;
	}


	switch (p_ts_header->pid) {
	case PID_PAT:
		p_ts_header->dump();
		break;

	case PID_EIT_H:
		mEIT_H.checkSection (p_ts_header, p_payload, payload_size);
		break;

	default:	
		break;
	}




	return true;
}
