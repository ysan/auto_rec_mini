#ifndef _THREADMGR_H_
#define _THREADMGR_H_

#include <stdbool.h>

#include "threadmgr_if.h"


/*
 * Constant define
 */


/*
 * Type define
 */


/*
 * External
 */
bool setup (const threadmgr_reg_tbl_t *p_tbl, uint8_t nr_tbl_max);
bool request_sync (uint8_t thread_idx, uint8_t seq_idx, uint8_t *msg, size_t msg_size);
bool request_async (uint8_t thread_idx, uint8_t seq_idx, uint8_t *msg, size_t msg_size, uint32_t *out_req_id);
void set_request_option (uint32_t option);
uint32_t get_request_option (void);
bool create_external_cp (void);
void destroy_external_cp (void);
threadmgr_src_info_t *receive_external (void);
void finaliz (void);
void wait_all (void);
void set_dispatcher (const PFN_DISPATCHER); /* for c++ wrapper extension */

#endif
