#ifndef _THREADMGR_IF_H_
#define _THREADMGR_IF_H_

#include <stdio.h>
#include <stdbool.h>

/*
 * Constant define
 */
#define THM_SECT_ID_INIT	(0)

// REQUEST_OPTION 16bit
// upper 16 bits are timeout value.
#define REQUEST_OPTION__WITHOUT_REPLY		(0x00000001)
#define REQUEST_OPTION__WITH_TIMEOUT_MSEC	(0x00000002)
#define REQUEST_OPTION__WITH_TIMEOUT_MIN	(0x00000004)
//#define REQUEST_OPTION__xxx			(0x00000008)
//#define REQUEST_OPTION__xxx			(0x00000010)
//#define REQUEST_OPTION__xxx			(0x00000020)
//#define REQUEST_OPTION__xxx			(0x00000040)
//#define REQUEST_OPTION__xxx			(0x00000080)
//#define REQUEST_OPTION__xxx			(0x00000100)
//#define REQUEST_OPTION__xxx			(0x00000200)
//#define REQUEST_OPTION__xxx			(0x00000400)
//#define REQUEST_OPTION__xxx			(0x00000800)
//#define REQUEST_OPTION__xxx			(0x00001000)
//#define REQUEST_OPTION__xxx			(0x00002000)
//#define REQUEST_OPTION__xxx			(0x00004000)
//#define REQUEST_OPTION__xxx			(0x00008000)




//TODO
#define _NO_TYPEDEF_uint64_t


/*
 * Type define
 */
#if !defined (_NO_TYPEDEF_uint8_t)
typedef unsigned char uint8_t;
#endif

#if !defined (_NO_TYPEDEF_uint16_t)
typedef unsigned short uint16_t;
#endif

#if !defined (_NO_TYPEDEF_uint32_t)
typedef unsigned int uint32_t;
#endif

#if !defined (_NO_TYPEDEF_uint64_t)
typedef unsigned long int uint64_t;
#endif

typedef enum {
	EN_THM_RSLT_IGNORE = 0,
	EN_THM_RSLT_SUCCESS,
	EN_THM_RSLT_ERROR,
	EN_THM_RSLT_REQ_TIMEOUT,
	EN_THM_RSLT_SEQ_TIMEOUT,
	EN_THM_RSLT_MAX,
} EN_THM_RSLT;

typedef enum {
	EN_THM_ACT_INIT = 0,
	EN_THM_ACT_CONTINUE,
	EN_THM_ACT_WAIT,
	EN_THM_ACT_DONE,
	EN_THM_ACT_MAX,
} EN_THM_ACT;

/* for c++ wrapper extension */
typedef enum {
	EN_THM_DISPATCH_TYPE_CREATE = 0,
	EN_THM_DISPATCH_TYPE_DESTROY,
	EN_THM_DISPATCH_TYPE_REQ_REPLY,
	EN_THM_DISPATCH_TYPE_NOTIFY,
	EN_THM_DISPATCH_TYPE_MAX,
} EN_THM_DISPATCH_TYPE;


typedef struct threadmgr_src_info {
	uint8_t thread_idx;
	uint8_t seq_idx;
	uint32_t req_id;
	EN_THM_RSLT result;
	uint8_t client_id;
	struct {
		uint8_t *msg;
		size_t size;
	} msg;
} threadmgr_src_info_t;


/*--- threadmgr_external_if ---*/
typedef struct threadmgr_external_if {
	bool (*request_sync) (uint8_t thread_idx, uint8_t seq_idx, uint8_t *msg, size_t msg_size);
	bool (*request_async) (uint8_t thread_idx, uint8_t seq_idx, uint8_t *msg, size_t msg_size, uint32_t *p_out_req_id);
	void (*set_request_option) (uint32_t option);
	uint32_t (*get_request_option) (void);
	bool (*create_external_cp) (void);
	void (*destroy_external_cp) (void);
	threadmgr_src_info_t* (*receive_external) (void);
} threadmgr_external_if_t;



/*--- threadmgr_if ---*/
typedef struct threadmgr_if {
	threadmgr_src_info_t *src_info;

	bool (*reply) (EN_THM_RSLT result, uint8_t *msg, size_t msg_size);

	bool (*reg_notify) (uint8_t category, uint8_t *pclient_id);
	bool (*unreg_notify) (uint8_t category, uint8_t client_id);

	bool (*notify) (uint8_t category, uint8_t *msg, size_t msg_size);

	void (*set_sectid) (uint8_t sect_id, EN_THM_ACT en_act);
	uint8_t (*get_sectid) (void);

	void (*set_timeout) (uint32_t timeout_msec);
	void (*clear_timeout) (void);

	void (*enable_overwrite) (void);
	void (*disable_overwrite) (void);

	void (*lock) (void);
	void (*unlock) (void);

	uint8_t (*get_seq_idx) (void);
	const char* (*get_seq_name) (void);
} threadmgr_if_t;


/*--- threadmgr_reg_tbl ---*/
typedef struct threadmgr_seq {
	void (*pcb_seq) (threadmgr_if_t *p_if);
	const char *name; // must be non-null
} threadmgr_seq_t;


typedef struct threadmgr_reg_tbl {
	const char *name;  // must be non-null
	void (*create) (void);
	void (*destroy) (void);
	uint8_t nr_que_max;
	const threadmgr_seq_t *seq_array;
	uint8_t nr_seq_max;
	void (*recv_notify) (threadmgr_if_t *p_if);
} threadmgr_reg_tbl_t;


/* for c++ wrapper extension */
typedef void (*PFN_DISPATCHER) (EN_THM_DISPATCH_TYPE en_type, uint8_t thread_idx, uint8_t seq_idx, threadmgr_if_t *p_if);


#ifdef __cplusplus
extern "C" {
#endif

/*
 * External
 */
extern threadmgr_external_if_t *setup_threadmgr (const threadmgr_reg_tbl_t *p_tbl, uint8_t n_tbl_max);
extern void wait_threadmgr (void);
extern void teardown_threadmgr (void);

extern void setup_dispatcher (const PFN_DISPATCHER pfn_dispatcher); /* for c++ wrapper extension */

#ifdef __cplusplus
};
#endif

#endif
