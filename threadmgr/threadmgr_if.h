#ifndef _THREADMGR_IF_H_
#define _THREADMGR_IF_H_

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
	uint32_t nReqId;
	EN_THM_RSLT enRslt;
	uint8_t nClientId;
	struct {
		uint8_t *pMsg;
		size_t size;
	} msg;
} ST_THM_SRC_INFO;


/*--- threadmgr_external_if ---*/
typedef bool (*PFN_REQUEST_SYNC) (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize);
typedef bool (*PFN_REQUEST_ASYNC) (uint8_t nThreadIdx, uint8_t nSeqIdx, uint8_t *pMsg, size_t msgSize, uint32_t *pOutReqId);
typedef void (*PFN_SET_REQUEST_OPTION) (uint32_t option);
typedef uint32_t (*PFN_GET_REQUEST_OPTION) (void);
typedef bool (*PFN_CREATE_EXTERNAL_CP) (void);
typedef void (*PFN_DESTROY_EXTERNAL_CP) (void);
typedef ST_THM_SRC_INFO* (*PFN_RECEIVE_EXTERNAL) (void);

typedef struct threadmgr_external_if {
	PFN_REQUEST_SYNC pfnRequestSync;
	PFN_REQUEST_ASYNC pfnRequestAsync;
	PFN_SET_REQUEST_OPTION pfnSetRequestOption;
	PFN_GET_REQUEST_OPTION pfnGetRequestOption;
	PFN_CREATE_EXTERNAL_CP pfnCreateExternalCp;
	PFN_DESTROY_EXTERNAL_CP pfnDestroyExternalCp;
	PFN_RECEIVE_EXTERNAL pfnReceiveExternal;
} ST_THM_EXTERNAL_IF;



/*--- threadmgr_if ---*/
typedef bool (*PFN_REPLY) (EN_THM_RSLT enRslt, uint8_t *pMsg, size_t msgSize);
typedef bool (*PFN_REG_NOTIFY) (uint8_t nCategory, uint8_t *pnClientId);
typedef bool (*PFN_UNREG_NOTIFY) (uint8_t nCategory, uint8_t nClientId);
typedef bool (*PFN_NOTIFY) (uint8_t nCategory, uint8_t *pMsg, size_t msgSize);
typedef void (*PFN_SET_SECTID) (uint8_t nSectId, EN_THM_ACT enAct);
typedef uint8_t (*PFN_GET_SECTID) (void);
typedef void (*PFN_SET_TIMEOUT) (uint32_t nTimeoutMsec);
typedef void (*PFN_CLEAR_TIMEOUT) (void);
typedef void (*PFN_ENABLE_OVERWRITE) (void);
typedef void (*PFN_DISABLE_OVERWRITE) (void);
typedef void (*PFN_LOCK) (void);
typedef void (*PFN_UNLOCK) (void);
typedef uint8_t (*PFN_GET_SEQ_IDX) (void);
typedef const char* (*PFN_GET_SEQ_NAME) (void);

typedef struct threadmgr_if {
	ST_THM_SRC_INFO *pstSrcInfo;

	PFN_REPLY pfnReply;

	PFN_REG_NOTIFY pfnRegNotify;
	PFN_UNREG_NOTIFY pfnUnRegNotify;
	PFN_NOTIFY pfnNotify;

	PFN_SET_SECTID pfnSetSectId;
	PFN_GET_SECTID pfnGetSectId;

	PFN_SET_TIMEOUT pfnSetTimeout;
	PFN_CLEAR_TIMEOUT pfnClearTimeout;

	PFN_ENABLE_OVERWRITE pfnEnableOverwrite;
	PFN_DISABLE_OVERWRITE pfnDisableOverwrite;

	PFN_LOCK pfnLock;
	PFN_UNLOCK pfnUnlock;

	PFN_GET_SEQ_IDX pfnGetSeqIdx;
	PFN_GET_SEQ_NAME pfnGetSeqName;

} ST_THM_IF;


/*--- threadmgr_reg_tbl ---*/
typedef void (*PCB_CREATE) (void);
typedef void (*PCB_DESTROY) (void);
typedef void (*PCB_THM_SEQ) (ST_THM_IF *pIf);
typedef void (*PCB_RECV_ASYNC_REPLY) (ST_THM_IF *pIf);
typedef void (*PCB_RECV_NOTIFY) (ST_THM_IF *pIf);

typedef struct threadmgr_seq {
	const PCB_THM_SEQ pcbSeq;
	const char *pszName; // must be non-null
} ST_THM_SEQ;


typedef struct threadmgr_reg_tbl {
	const char *pszName;  // must be non-null
	const PCB_CREATE pcbCreate;
	const PCB_DESTROY pcbDestroy;
	uint8_t nQueNum;
	const ST_THM_SEQ *pstSeqArray;
	uint8_t nSeqNum;
	const PCB_RECV_NOTIFY pcbRecvNotify;
} ST_THM_REG_TBL;


/* for c++ wrapper extension */
typedef void (*PFN_DISPATCHER) (EN_THM_DISPATCH_TYPE enType, uint8_t nThreadIdx, uint8_t nSeqIdx, ST_THM_IF *pIf);


#ifdef __cplusplus
extern "C" {
#endif

/*
 * External
 */
extern ST_THM_EXTERNAL_IF *setupThreadMgr (const ST_THM_REG_TBL *pTbl, uint8_t nTblMax);
extern void teardownThreadMgr (void);
extern void setupDispatcher (const PFN_DISPATCHER pfnDispatcher); /* for c++ wrapper extension */

#ifdef __cplusplus
};
#endif

#endif
