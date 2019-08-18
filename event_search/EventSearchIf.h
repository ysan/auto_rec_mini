#ifndef _EVENT_SEARCH_IF_H_
#define _EVENT_SEARCH_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_EVENT_SEARCH__MODULE_UP = 0,
	EN_SEQ_EVENT_SEARCH__MODULE_DOWN,
	EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH,
	EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH_EX,

	EN_SEQ_EVENT_SEARCH__NUM,
};


class CEventSearchIf : public CThreadMgrExternalIf
{
public:
	explicit CEventSearchIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CEventSearchIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_EVENT_SEARCH, EN_SEQ_EVENT_SEARCH__MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_EVENT_SEARCH, EN_SEQ_EVENT_SEARCH__MODULE_DOWN);
	};

	bool reqAddRecReserve_keywordSearch (void) {
		return requestAsync (EN_MODULE_EVENT_SEARCH, EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH);
	};

	bool reqAddRecReserve_keywordSearch_ex (void) {
		return requestAsync (EN_MODULE_EVENT_SEARCH, EN_SEQ_EVENT_SEARCH__ADD_REC_RESERVE__KEYWORD_SEARCH_EX);
	};

};

#endif
