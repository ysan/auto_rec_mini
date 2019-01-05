#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include "threadmgr.h"
#include "threadmgr_if.h"
#include "threadmgr_util.h"


/*
 * Constant define
 */


/*
 * Type define
 */


/*
 * Variables
 */


/*
 * Prototypes
 */
ST_THM_EXTERNAL_IF *setupThreadMgr (const ST_THM_REG_TBL *pTbl, uint8_t nTblMax); // extern
void teardownThreadMgr (void); // extern


/* threadMgr external object */
static ST_THM_EXTERNAL_IF s_stThmExternalIf = {
	requestSync,
	requestAsync,
	createExternalCp,
	destroyExternalCp,
	receiveExternal,
};


/**
 * setupThreadMgr
 */
ST_THM_EXTERNAL_IF *setupThreadMgr (const ST_THM_REG_TBL *pTbl, uint8_t nTblMax)
{
	if (!setup (pTbl, nTblMax)) {
		return NULL;
	}

	return &s_stThmExternalIf;
}

/**
 * teardownThreadMgr
 */
void teardownThreadMgr (void)
{
	finalize ();
}

/**
 * setDisaptcher
 * for c++ wrapper extention
 */
void setupDispatcher (const PFN_DISPATCHER pfnDispatcher)
{
	if (pfnDispatcher) {
		setDispatcher (pfnDispatcher);
	}
}
