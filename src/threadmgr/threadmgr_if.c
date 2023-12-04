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
threadmgr_external_if_t *setup_threadmgr (const threadmgr_reg_tbl_t *p_tbl, uint8_t nr_tbl_max); // extern
void wait_threadmgr (void); // extern
void teardown_threadmgr (void); // extern


/* threadmgr external object */
static threadmgr_external_if_t s_thm_external_if = {
	request_sync,
	request_async,
	set_request_option,
	get_request_option,
	create_external_cp,
	destroy_external_cp,
	receive_external,
};


/**
 * setup_threadmgr
 */
threadmgr_external_if_t *setup_threadmgr (const threadmgr_reg_tbl_t *p_tbl, uint8_t nr_tbl_max)
{
	if (!setup (p_tbl, nr_tbl_max)) {
		return NULL;
	}

	return &s_thm_external_if;
}

/**
 * wait_threadmgr
 */
void wait_threadmgr (void)
{
	wait_all ();
}

/**
 * teardown_threadmgr
 */
void teardown_threadmgr (void)
{
	finaliz ();
}


/**
 * set_disaptcher
 * for c++ wrapper extention
 */
void setup_dispatcher (const PFN_DISPATCHER pfn_dispatcher)
{
	if (pfn_dispatcher) {
		set_dispatcher (pfn_dispatcher);
	}
}
