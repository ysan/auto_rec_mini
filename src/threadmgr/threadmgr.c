/*
 * 簡易スレッドマネージャ
 */
//TODO 終了前に全threadのbacktrace
//TODO req_id類の配列は THREAD_IDX_MAX +1でもつのがわかりにくい
//TODO base_threadで workerキューの状態check
//TODO receive_externalで reqtimeout考慮 ->base_threadでのチェック廃止
//TODO check_wait_worker_thread ここでque getoutリターンしてしまえばいいのでは
//		check2deque_workerのgetout falseチェックはいらない


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <semaphore.h>
#include <limits.h>

#include "threadmgr.h"
#include "threadmgr_if.h"
#include "threadmgr_util.h"


/*
 * Constant define
 */
#define SYS_RETURN_NORMAL				(0)

#define THREAD_NAME_SIZE				(16)
#define BASE_THREAD_NAME				"THMbase"
#define SIGWAIT_THREAD_NAME				"THMsigwait"

#define THREAD_IDX_MAX					(0x20) // 32  uint8の為 255以下の値で THREAD_IDX_BLANKより小さい値
#define THREAD_IDX_BLANK				(0x80) // 128 uint8の為 255以下の値
#define THREAD_IDX_EXTERNAL				THREAD_IDX_MAX // req_id類の配列は THREAD_IDX_MAX +1で確保します

#define SEQ_IDX_MAX						(0x40) // 64  uint8の為 255以下の値で SEQ_IDX_BLANKより小さい値
#define SEQ_IDX_BLANK					(0x80) // 128 uint8の為 255以下の値

#define QUE_BASE_NUM					(64)
#define QUE_WORKER_MIN					(5)
#define QUE_WORKER_MAX					(100)

#define REQUEST_ID_MAX					(0x40) // REQUEST_ID_BLANK 以下の値で 2のべき乗の値
#define REQUEST_ID_BLANK				(0x80)
#define REQUEST_ID_UNNECESSARY			(0x81)

#define REQUEST_TIMEOUT_MAX				(0x05265C00) // 24時間 msec
// for debug
#define REQUEST_TIMEOUT_10				(10000) // msec
#define REQUEST_TIMEOUT_30				(30000) // msec
#define REQUEST_TIMEOUT_60				(60000) // msec
#define REQUEST_TIMEOUT_FIX				REQUEST_TIMEOUT_30

#define MSG_SIZE						(256)

#define NOTIFY_CATEGORY_MAX				(0x20) // 32  notifyを登録できるカテゴリ数 (notifyの種別)
#define NOTIFY_CATEGORY_BLANK			(0x80) // 128
#define NOTIFY_CLIENT_ID_MAX			(0xa0) // 160  notifyを登録できるクライアント数 全スレッドに渡りユニーク
#define NOTIFY_CLIENT_ID_BLANK			(0xe0) // 224

#define SECT_ID_MAX						(0x40) // 64  1sequenceあたりsection分割可能な最大数
#define SECT_ID_BLANK					(0x80) // 128
#define SECT_ID_INIT					THM_SECT_ID_INIT

#define SEQ_TIMEOUT_BLANK				(0)
#define SEQ_TIMEOUT_MAX					(0x05265C00) // 24時間 msec

#define BASE_THREAD_LOOP_TIMEOUT_SEC	(5)



/*
 * Type define
 */
typedef enum {
	EN_STATE_INIT = 0,
	EN_STATE_READY,
	EN_STATE_BUSY,
	EN_STATE_WAIT_REPLY, // for request_sync
	EN_STATE_DESTROY,
	EN_STATE_MAX
} EN_STATE;

typedef enum {
	EN_MONI_TYPE_INIT = 0,
	EN_MONI_TYPE_DEUnexpected,
	EN_MONI_TYPE_DESTROY,

} EN_MONI_TYPE;

typedef enum {
	EN_QUE_TYPE_INIT = 0,
	EN_QUE_TYPE_REQUEST,
	EN_QUE_TYPE_REPLY,
	EN_QUE_TYPE_NOTIFY,
	EN_QUE_TYPE_SEQ_TIMEOUT,
	EN_QUE_TYPE_REQ_TIMEOUT,
	EN_QUE_TYPE_DESTROY,
	EN_QUE_TYPE_MAX
} EN_QUE_TYPE;

typedef enum {
	EN_TIMEOUT_STATE_INIT = 0,
	EN_TIMEOUT_STATE_MEAS,
	EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT,
	EN_TIMEOUT_STATE_PASSED,
	EN_TIMEOUT_STATE_NOT_SET,
	EN_TIMEOUT_STATE_MAX,
} EN_TIMEOUT_STATE;

typedef enum {
	EN_NEAREST_TIMEOUT_NONE = 0,
	EN_NEAREST_TIMEOUT_REQ,
	EN_NEAREST_TIMEOUT_SEQ,
} EN_NEAREST_TIMEOUT;



typedef struct que_base {
	EN_MONI_TYPE en_moni_type;
	bool is_used;
} que_base_t;

typedef struct que_worker {
	uint8_t dest_thread_idx;
	uint8_t dest_seq_idx;

	/* context */
	bool is_valid_src_info;
	uint8_t src_thread_idx;
	uint8_t src_seq_idx;

	EN_QUE_TYPE en_que_type;

	/* for Request */
	uint32_t req_id;

	/* for Reply */
	EN_THM_RSLT en_rslt;

	/* for Notify */
	uint8_t client_id;

	/* message */
	struct {
		uint8_t msg [MSG_SIZE];
		size_t size;
		bool is_used;
	} msg;

	bool is_used;
	bool is_drop;

} que_worker_t;

typedef struct context {
	bool is_valid;
	uint8_t thread_idx;
	uint8_t seq_idx;
} context_t;

/* 登録したシーケンスごとに紐つく情報 */
typedef struct seq_info {
	uint8_t seq_idx;

	/* 以下可変情報 */

	uint8_t sect_id;
	EN_THM_ACT en_act;
#ifndef _MULTI_REQUESTING
	uint32_t req_id; // シーケンス中にrequestしたときのreq_id  replyが返ってきたとき照合する
#endif
	que_worker_t seq_init_que_worker; // このシーケンスを開始させたrequestキューを保存します
	bool is_overwrite;
	bool is_lock;

	/* Seqタイムアウト情報 */
	struct {
		EN_TIMEOUT_STATE en_state;
		uint32_t val; // unit:m_s
		struct timespec time;
	} timeout;

	uint8_t running_sect_id; // 現在実行中のセクションid dump用

} seq_info_t;

/* スレッド内部情報 (per thread) */
typedef struct inner_info {
	uint8_t thread_idx;
	char *name;
	pthread_t pthread_id;
	pid_t tid;
	uint8_t nr_que_worker;
	que_worker_t *que_worker; // nr_que_worker数分の配列をさします
	uint8_t nr_seq;
	seq_info_t *seq_info; // nr_seq数分の配列をさします

	/* 以下可変情報 */

	EN_STATE en_state;
	que_worker_t now_exec_que_worker;

	uint32_t request_option;
	uint32_t requetimeout_msec;

} inner_info_t;

typedef struct sync_reply_info {
	uint32_t req_id;
	EN_THM_RSLT en_rslt;

	/* message */
	struct {
		uint8_t msg [MSG_SIZE];
		size_t size;
		bool is_used;
	} msg;

	bool is_reply_already;
	bool is_used;

} sync_reply_info_t;

/* request_id 1つごとに紐つく情報 */
typedef struct request_id_info {
	uint32_t id;

	uint8_t src_thread_idx;
	uint8_t src_seq_idx;

	/* Reqタイムアウト情報 */
	struct {
		EN_TIMEOUT_STATE en_state;
		struct timespec time;
	} timeout;

} request_id_info_t;

typedef struct notify_client_info {
	uint8_t src_thread_idx; // server thread_idx
	uint8_t dest_thread_idx; // client thread_idx
	uint8_t category;
	bool is_used;
} notify_client_info_t;

typedef struct external_control_info {
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	pthread_t pthread_id; // key
	uint32_t req_id;

	struct {
		uint8_t msg [MSG_SIZE];
		size_t size;
	} msg_entity;

	threadmgr_src_info_t thm_src_info;

	bool is_reply_already;

	uint32_t request_option;
	uint32_t requetimeout_msec;


	struct external_control_info *next;

} external_control_info_t;


/*
 * Variables
 */
static threadmgr_reg_tbl_t *gp_thm_reg_tbl [THREAD_IDX_MAX];
static inner_info_t g_inner_info [THREAD_IDX_MAX];

static uint8_t g_nr_total_worker_thread;

static que_base_t g_que_base [QUE_BASE_NUM];

static uint32_t g_nr_request_id_ind [THREAD_IDX_MAX +1]; // 添え字 +1 は外部のスレッドからのリクエスト用
static request_id_info_t g_request_id_info [THREAD_IDX_MAX +1][REQUEST_ID_MAX]; // 添え字 +1 は外部のスレッドからのリクエスト用

static sync_reply_info_t g_sync_reply_info [THREAD_IDX_MAX];
static threadmgr_src_info_t g_thm_src_info [THREAD_IDX_MAX];

static notify_client_info_t g_notify_client_info [NOTIFY_CLIENT_ID_MAX];

static external_control_info_t *g_ext_info_list_top;
static external_control_info_t *g_ext_info_list_btm;

static pthread_mutex_t g_mutex_base;
static pthread_cond_t g_cond_base;
static pthread_mutex_t g_mutex_worker [THREAD_IDX_MAX];
static pthread_cond_t g_cond_worker [THREAD_IDX_MAX];
static pthread_mutex_t g_mutex_sync_reply [THREAD_IDX_MAX];
static pthread_cond_t g_cond_sync_reply [THREAD_IDX_MAX];
static pthread_mutex_t g_mutex_ope_que_base;
static pthread_mutex_t g_mutex_ope_que_worker [THREAD_IDX_MAX];
static pthread_mutex_t g_mutex_ope_request_id [THREAD_IDX_MAX +1]; // 添え字 +1 は外部のスレッドからのリクエスト用
//static pthread_mutex_t g_mutex_notify_client_info [THREAD_IDX_MAX];
static pthread_mutex_t g_mutex_notify_client_info ; // g_mutex_notify_client_infoのmutexはプロセスにわたりユニークにアクセスするため一つになります
static pthread_mutex_t g_mutex_ope_ext_info_list;

static sigset_t g_sigset; // プロセス全体のシグナルセット
static sem_t g_sem;

static pid_t g_tid_base_thread = 0;
static pid_t g_tid_sig_wait_thread = 0;

static bool g_is_enable_log = false;

static const char *gpsz_state [EN_STATE_MAX] = {
	// for debug log
	"STATE_INIT",
	"STATE_READY",
	"STATE_BUSY",
	"STATE_WAIT_REPLY",
	"STATE_DESTROY",
};
static const char *gpsz_que_type [EN_QUE_TYPE_MAX] = {
	// for debug log
	"TYPE_INIT",
	"TYPE_REQ",
	"TYPE_REPLY",
	"TYPE_NOTIFY",
	"TYPE_SEQ_TIMEOUT",
	"TYPE_REQ_TIMEOUT",
	"TYPE_DESTROY",
};
static const char *gpsz_rslt [EN_THM_RSLT_MAX] = {
	// for debug log
	"RSLT_IGNORE",
	"RSLT_SUCCESS",
	"RSLT_ERROR",
	"RSLT_REQ_TIMEOUT",
	"RSLT_SEQ_TIMEOUT",
};
static const char *gpsz_act [EN_THM_ACT_MAX] = {
	// for debug log
	"ACT_INIT",
	"ACT_CONTINUE",
	"ACT_WAIT",
	"ACT_DONE",
};
static const char *gpsz_timeout_state [EN_TIMEOUT_STATE_MAX] = {
	// for debug log
	"TIMEOUT_STATE_INIT",
	"TIMEOUT_STATE_MEAS",
	"TIMEOUT_STATE_MEAS_COND_WAIT",
	"TIMEOUT_STATE_PASSED",
	"TIMEOUT_STATE_NOT_SET",
};

static PFN_DISPATCHER gpfn_dispatcher = NULL; /* for c++ wrapper extension */


/*
 * Prototypes
 */
bool setup (const threadmgr_reg_tbl_t *p_tbl, uint8_t n_tbl_max); // extern
static void init (void);
static void init_que (void);
static void init_cond_mutex (void);
static void setup_signal (void);
static void finaliz_signal (void);
static void setup_signal_per_thread (void);
static void finaliz_signal_per_thread (void);
static void setup_sem (void);
static void finaliz_sem (void);
static void post_sem (void);
static void wait_sem (void);
static uint8_t get_total_worker_thread_num (void);
static void set_total_worker_thread_num (uint8_t n);
static bool register_threadmgr_tbl (const threadmgr_reg_tbl_t *p_tbl, uint8_t nr_tbl_max);
static void dump_inner_info (void);
static void clear_inner_info (inner_info_t *p);
static EN_STATE get_state (uint8_t thread_idx);
static void set_state (uint8_t thread_idx, EN_STATE en_state);
static void set_thread_name (char *p);
static pid_t get_task_id (void);
static bool is_exist_thread (pid_t tid);
static void check_worker_thread (void);
static bool create_base_thread (void);
static bool create_worker_thread (uint8_t thread_idx);
static bool create_all_thread (void);
static bool enque_base (EN_MONI_TYPE en_type);
static que_base_t deque_base (bool is_get_out);
static void clear_que_base (que_base_t *p);
static bool enque_worker (
	uint8_t thread_idx,
	uint8_t seq_idx,
	EN_QUE_TYPE en_que_type,
	context_t *context,
	uint32_t req_id,
	EN_THM_RSLT en_rslt,
	uint8_t client_id,
	uint8_t *msg,
	size_t msg_size
);
static void dump_que_worker (uint8_t thread_idx);
static void dump_que_all_thread (void);
//static que_worker_t deque_worker (uint8_t thread_idx, bool is_get_out);
static que_worker_t check2deque_worker (uint8_t thread_idx, bool is_get_out);
static void clear_que_worker (que_worker_t *p);
static void *base_thread (void *arg);
static void *sigwait_thread (void *arg);
static void check_wait_worker_thread (inner_info_t *inner_info);
static void *worker_thread (void *arg);
static void clear_thm_if (threadmgr_if_t *p_if);
static void clear_thm_src_info (threadmgr_src_info_t *p);
static bool request_base_thread (EN_MONI_TYPE en_type);
static bool request_inner (
	uint8_t thread_idx,
	uint8_t seq_idx,
	uint32_t req_id,
	context_t *context,
	uint8_t *msg,
	size_t msg_size
);
bool request_sync (uint8_t thread_idx, uint8_t seq_idx, uint8_t *msg, size_t msg_size); // extern
bool request_async (uint8_t thread_idx, uint8_t seq_idx, uint8_t *msg, size_t msg_size, uint32_t *out_req_id); // extern
void set_request_option (uint32_t option); // extern
uint32_t get_request_option (void); // extern
static uint32_t apply_timeout_msec_by_request_option (uint32_t option);
static bool reply_inner (
	uint8_t thread_idx,
	uint8_t seq_idx,
	uint32_t req_id,
	context_t *context,
	EN_THM_RSLT en_rslt,
	uint8_t *msg,
	size_t msg_size,
	bool is_sync
);
static bool reply_outer (
	uint32_t req_id,
	context_t *context,
	EN_THM_RSLT en_rslt,
	uint8_t *msg,
	size_t msg_size
);
static bool reply (EN_THM_RSLT en_rslt, uint8_t *msg, size_t msg_size);
static context_t get_context (void);
static void clear_context (context_t *p);
static uint32_t get_request_id (uint8_t thread_idx, uint8_t seq_idx);
static void dump_request_id_info (void);
static request_id_info_t *get_request_id_info (uint8_t thread_idx, uint32_t req_id);
static bool is_active_request_id (uint8_t thread_idx, uint32_t req_id);
static void enable_req_timeout (uint8_t thread_idx, uint32_t req_id, uint32_t timeout_msec);
static void check_req_timeout (uint8_t thread_idx);
static bool is_req_timeout_from_request_id (uint8_t thread_idx, uint32_t req_id);
static bool enque_req_timeout (uint8_t thread_idx, uint32_t req_id);
static EN_TIMEOUT_STATE get_req_timeout_state (uint8_t thread_idx, uint32_t req_id);
static request_id_info_t *search_nearest_req_timeout (uint8_t thread_idx);
static void release_request_id (uint8_t thread_idx, uint32_t req_id);
static void clear_request_id_info (request_id_info_t *p);
static bool is_sync_reply_from_request_id (uint8_t thread_idx, uint32_t req_id);
static bool set_request_id_sync_reply (uint8_t thread_idx, uint32_t req_id);
static sync_reply_info_t *get_sync_reply_info (uint8_t thread_idx);
static bool cache_sync_reply_info (uint8_t thread_idx, EN_THM_RSLT en_rslt, uint8_t *msg, size_t msg_size);
static void set_reply_already_sync_reply_info (uint8_t thread_idx);
static void clear_sync_reply_info (sync_reply_info_t *p);
static bool register_notify (uint8_t category, uint8_t *pclient_id);
static bool unregister_notify (uint8_t category, uint8_t client_id);
static uint8_t set_notify_client_info (uint8_t thread_idx, uint8_t category, uint8_t client_thread_idx);
static bool unset_notify_client_info (uint8_t thread_idx, uint8_t category, uint8_t client_id);
static bool notify_inner (
	uint8_t thread_idx,
	uint8_t client_id,
	context_t *context,
	uint8_t *msg,
	size_t msg_size
);
static bool notify (uint8_t category, uint8_t *msg, size_t msg_size);
static void dump_notify_client_info (void);
static void clear_notify_client_info (notify_client_info_t *p);
static void set_sect_id (uint8_t n_sect_id, EN_THM_ACT en_act);
static void set_sect_id_inner (uint8_t thread_idx, uint8_t seq_idx, uint8_t n_sect_id, EN_THM_ACT en_act);
static void clear_sect_id (uint8_t thread_idx, uint8_t seq_idx);
static uint8_t get_sect_id (void);
static uint8_t get_seq_idx (void);
static const char* get_seq_name (void);
static void set_timeout (uint32_t timeout_msec);
static void set_seq_timeout (uint32_t timeout_msec);
static void enable_seq_timeout (uint8_t thread_idx, uint8_t seq_idx);
static void check_seq_timeout (uint8_t thread_idx);
static bool is_seq_timeout_from_seq_idx (uint8_t thread_idx, uint8_t seq_idx);
static bool enque_seq_timeout (uint8_t thread_idx, uint8_t seq_idx);
static seq_info_t *search_nearest_seq_timeout (uint8_t thread_idx);
static void check_seq_timeout_from_cond_timedwait (uint8_t thread_idx);
static void check_seq_timeout_from_not_cond_timedwait (uint8_t thread_idx);
static void clear_timeout (void);
static void clear_seq_timeout (uint8_t thread_idx, uint8_t seq_idx);
static seq_info_t *get_seq_info (uint8_t thread_idx, uint8_t seq_idx);
static void clear_seq_info (seq_info_t *p);
static void enable_overwrite (void);
static void disable_overwrite (void);
static void set_overwrite (uint8_t thread_idx, uint8_t seq_idx, bool is_overwrite);
static bool is_lock (uint8_t thread_idx);
static bool is_lock_seq (uint8_t thread_idx, uint8_t seq_idx);
static void lock (void);
static void unlock (void);
static void set_lock (uint8_t thread_idx, uint8_t seq_idx, bool is_lock);
static EN_NEAREST_TIMEOUT search_nearetimeout (
	uint8_t thread_idx,
	request_id_info_t **p_request_id_info,	// out
	seq_info_t **p_seq_info				// out
);
static void add_ext_info_list (external_control_info_t *ext_info);
static external_control_info_t *search_ext_info_list (pthread_t key);
static external_control_info_t *search_ext_info_list_from_request_id (uint32_t req_id);
static void delete_ext_info_list (pthread_t key);
static void dump_ext_info_list (void);
bool create_external_cp (void); // extern
void destroy_external_cp (void); // extern
threadmgr_src_info_t *receive_external (void); // extern
static void clear_external_control_info (external_control_info_t *p);
void finaliz (void); // extern
static bool destroy_inner (uint8_t thread_idx);
static bool destroy_worker_thread (uint8_t thread_idx);
static bool destroy_all_worker_thread (void);
void wait_all (void); // extern
static void wait_base_thread (void);
static void wait_worker_thread (uint8_t thread_idx);
static bool is_enable_log (void);
static void enable_log (void);
static void disable_log (void);
void set_dispatcher (const PFN_DISPATCHER pfn_dispatcher); /* for c++ wrapper extension */ // extern


/*
 * inner log macro
 */
#define THM_INNER_FORCE_LOG_D(fmt, ...) do {\
	__puts_log (get_log_fileptr(), EN_LOG_TYPE_D, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#define THM_INNER_FORCE_LOG_I(fmt, ...) do {\
	__puts_log (get_log_fileptr(), EN_LOG_TYPE_I, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#define THM_INNER_FORCE_LOG_W(fmt, ...) do {\
	__puts_log (get_log_fileptr(), EN_LOG_TYPE_W, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)

#ifdef _ENABLE_INNER_LOG
#define THM_INNER_LOG_D(fmt, ...) do {\
	if (is_enable_log()) {\
		__puts_log (get_log_fileptr(), EN_LOG_TYPE_D, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
	}\
} while (0)
#define THM_INNER_LOG_I(fmt, ...) do {\
	if (is_enable_log()) {\
		__puts_log (get_log_fileptr(), EN_LOG_TYPE_I, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
	}\
} while (0)
#define THM_INNER_LOG_W(fmt, ...) do {\
	if (is_enable_log()) {\
		__puts_log (get_log_fileptr(), EN_LOG_TYPE_W, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
	}\
} while (0)
#else // not _ENABLE_INNER_LOG
#define THM_INNER_LOG_D(fmt, ...) do {\
} while (0)
#define THM_INNER_LOG_I(fmt, ...) do {\
} while (0)
#define THM_INNER_LOG_W(fmt, ...) do {\
} while (0)
#endif

#define THM_INNER_LOG_E(fmt, ...) do {\
	__puts_log (get_log_fileptr(), EN_LOG_TYPE_E, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#define THM_INNER_PERROR(fmt, ...) do {\
	__puts_log (get_log_fileptr(), EN_LOG_TYPE_PE, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)


/**
 * シグナルハンドラ
 *
 * 参考: libc segfault.c
 */
static void _signal_handler (int signal)
{
	char name [32] = {0};
	get_thread_name (name, 32);
	THM_LOG_W ("\n_backtrace: %s\n", name);
	puts_backtrace();

	if (signal == SIGSEGV || signal == (SIGRTMIN +2)) {
		FILE *fp = fopen ("/proc/self/maps", "r");
		if (fp) {
			THM_LOG_W ("\n_memory map:\n\n");
			char buf [1024] = {0};
			while (!feof(fp)) {
				memset (buf, 0x00, sizeof(buf));
				fgets(buf, 1024, fp);
				THM_LOG_W (buf);
			}
			fclose(fp);
		}
	}

	if (signal == SIGSEGV) {
		struct sigaction sa;
		sa.sa_handler = SIG_DFL;
		sigemptyset (&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction (signal, &sa, NULL);
		raise (signal);
	}
}

/**
 * threadmgrの初期化とセットアップ
 * 最初にこれを呼びます
 */
bool setup (const threadmgr_reg_tbl_t *p_tbl, uint8_t n_tbl_max)
{
	if ((!p_tbl) || (n_tbl_max == 0)) {
		THM_INNER_LOG_E ("invalid argument.\n");
		return false;
	}

	init();

	setup_sem ();

	/* 以降の生成されたスレッドはこのシグナルマスクを継承する */
	setup_signal();

	if (!register_threadmgr_tbl (p_tbl, n_tbl_max)) {
		THM_INNER_LOG_E ("register_threadmgr_tbl() is failure.\n");
		return false;
	}

	if (get_total_worker_thread_num() == 0) {
		THM_INNER_LOG_E ("total thread is 0.\n");
		return false;
	}


	if (!create_all_thread()) {
		THM_INNER_LOG_E ("create_all_thread() is failure.\n");
		return false;
	}

	/* 全スレッド立ち上がるまで待つ */
	wait_sem ();
	THM_INNER_FORCE_LOG_I ("----- setup complete. -----\n");


	return true;
}

/**
 * 初期化処理
 * 
 */
static void init (void)
{
	int i = 0;
	int j = 0;


	for (i = 0; i < THREAD_IDX_MAX; ++ i) {
		gp_thm_reg_tbl [i] = NULL;
	}

	/* init inner_info */
	for (i = 0; i < THREAD_IDX_MAX; ++ i) {
		clear_inner_info (&g_inner_info[i]);
	}

	set_total_worker_thread_num (0);


	/* init request_id_info */
	for (i = 0; i < THREAD_IDX_MAX +1; ++ i) { // 添え字 +1 は外部のスレッドからのリクエスト用
		g_nr_request_id_ind [i] = 0;
		for (j = 0; j < REQUEST_ID_MAX; ++ j) {
			clear_request_id_info (&g_request_id_info [i][j]);
		}
	}

	/* init sync_reply_info  */
	for (i = 0; i < THREAD_IDX_MAX; ++ i) {
		clear_sync_reply_info (&g_sync_reply_info [i]);
	}

	/* init thm_src_info */
	for (i = 0; i < THREAD_IDX_MAX; ++ i) {
		clear_thm_src_info (&g_thm_src_info [i]);
	}

	/* init notify_info */
	for (i = 0; i < NOTIFY_CLIENT_ID_MAX; ++ i) {
		clear_notify_client_info (&g_notify_client_info [i]);
	}


	init_que();
	init_cond_mutex();


	/* lock */
	pthread_mutex_lock (&g_mutex_ope_ext_info_list);
	g_ext_info_list_top = NULL;
	g_ext_info_list_btm = NULL;
	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_ext_info_list);
}

/**
 * 初期化
 * キュー
 */
static void init_que (void)
{
	int i = 0;

	for (i = 0; i < QUE_BASE_NUM; ++ i) {
		clear_que_base (&g_que_base [i]);
	}
}

/**
 * 初期化
 * cond & mutex
 */
static void init_cond_mutex (void)
{
	int i = 0;

	pthread_mutex_init (&g_mutex_base, NULL);
	pthread_cond_init (&g_cond_base, NULL);

//TODO 再帰mutexを使用します
	pthread_mutexattr_t attr_mutex_worker;
	pthread_mutexattr_init (&attr_mutex_worker);
	pthread_mutexattr_settype (&attr_mutex_worker, PTHREAD_MUTEX_RECURSIVE_NP);
	for (i = 0; i < THREAD_IDX_MAX; ++ i) {
		pthread_mutex_init (&g_mutex_worker [i], &attr_mutex_worker);
		pthread_cond_init (&g_cond_worker [i], NULL);
	}

	for (i = 0; i < THREAD_IDX_MAX; ++ i) {
		pthread_mutex_init (&g_mutex_sync_reply [i], NULL);
		pthread_cond_init (&g_cond_sync_reply [i], NULL);
	}

	pthread_mutex_init (&g_mutex_ope_que_base, NULL);
	for (i = 0; i < THREAD_IDX_MAX; ++ i) {
		pthread_mutex_init (&g_mutex_ope_que_worker [i], NULL);
	}

//TODO 再帰mutexを使用します
	pthread_mutexattr_t attr_mutex_ope_request_id;
	pthread_mutexattr_init (&attr_mutex_ope_request_id);
	pthread_mutexattr_settype (&attr_mutex_ope_request_id, PTHREAD_MUTEX_RECURSIVE_NP);
	for (i = 0; i < THREAD_IDX_MAX +1; ++ i) { // 添え字 +1 は外部のスレッドからのリクエスト用
		pthread_mutex_init (&g_mutex_ope_request_id [i], &attr_mutex_ope_request_id);
	}

//	for (i = 0; i < THREAD_IDX_MAX; ++ i) {
//		pthread_mutex_init (&g_mutex_notify_client_info [i], NULL);
//	}
	pthread_mutex_init (&g_mutex_notify_client_info, NULL);

	pthread_mutex_init (&g_mutex_ope_ext_info_list, NULL);
}

/**
 * setup_signal
 * プロセス全体のシグナル設定
 */
static void setup_signal (void)
{
	sigemptyset (&g_sigset);

	sigaddset (&g_sigset, SIGQUIT); //TODO terminal (ctrl + \)
	sigaddset (&g_sigset, SIGINT);
	sigaddset (&g_sigset, SIGTERM);
	sigprocmask (SIG_BLOCK, &g_sigset, NULL);
}

/**
 * finaliz_signal
 */
static void finaliz_signal (void)
{
	sigprocmask (SIG_UNBLOCK, &g_sigset, NULL);
}

/**
 * setup_signal_per_thread
 * スレッドごとに行うシグナル設定
 *
 * 参考: libc segfault.c
 */
static void setup_signal_per_thread (void)
{
	struct sigaction sa;

	sa.sa_handler = (void *) _signal_handler;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;

	void *stack_mem = malloc (2 * SIGSTKSZ);
	stack_t ss;

	if (stack_mem != NULL) {
		ss.ss_sp = stack_mem;
		ss.ss_flags = 0;
		ss.ss_size = 2 * SIGSTKSZ;

		if (sigaltstack (&ss, NULL) == 0) {
			sa.sa_flags |= SA_ONSTACK;
		}
	}

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGRTMIN +1, &sa, NULL);
	sigaction(SIGRTMIN +2, &sa, NULL);
}

/**
 * finaliz_signal_per_thread
 */
static void finaliz_signal_per_thread (void)
{
	struct sigaction sa;
	sa.sa_handler = SIG_DFL;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (SIGSEGV, &sa, NULL);
	sigaction (SIGRTMIN +1, &sa, NULL);
	sigaction (SIGRTMIN +2, &sa, NULL);
}

/**
 * setup_sem
 */
static void setup_sem (void)
{
	sem_init (&g_sem, 0, 0); 
}

/**
 * finaliz_sem
 */
static void finaliz_sem (void)
{
	sem_destroy (&g_sem); 
}

/**
 * post_sem
 */
static void post_sem (void)
{
	sem_post (&g_sem);
}

/**
 * wait_sem
 */
static void wait_sem (void)
{
	uint8_t i = 0;

	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		sem_wait (&g_sem);
		THM_INNER_LOG_I ("sem_wait return\n");
	}

	/* base_threadとsigwait_threadの分 */
	sem_wait (&g_sem);
	THM_INNER_LOG_I ("sem_wait return\n");
	sem_wait (&g_sem);
	THM_INNER_LOG_I ("sem_wait return\n");
}

/**
 * get_total_worker_thread_num
 */
static uint8_t get_total_worker_thread_num (void)
{
	return g_nr_total_worker_thread;
}

/**
 * set_total_worker_thread_num
 */
static void set_total_worker_thread_num (uint8_t n)
{
	g_nr_total_worker_thread = n;
}

/**
 * register_threadmgr_tbl
 * スレッドテーブルを登録する
 */
static bool register_threadmgr_tbl (const threadmgr_reg_tbl_t *p_tbl, uint8_t nr_tbl_max)
{
	if ((!p_tbl) || (nr_tbl_max == 0)) {
		THM_INNER_LOG_E ("invalid argument.\n");
		return false;
	}

	int i = 0;

	while (i < nr_tbl_max) {
		if (i == THREAD_IDX_MAX) {
			// over flow
			THM_INNER_LOG_E ("thread idx is over. (inner thread table)\n");
			return false;
		}

		if ((((threadmgr_reg_tbl_t*)p_tbl)->nr_que < QUE_WORKER_MIN) && (((threadmgr_reg_tbl_t*)p_tbl)->nr_que > QUE_WORKER_MAX)) {
			THM_INNER_LOG_E ("que num is invalid. (inner thread table)\n");
			return false;
		}

		if ((((threadmgr_reg_tbl_t*)p_tbl)->nr_seq <= 0) && (((threadmgr_reg_tbl_t*)p_tbl)->nr_seq > SEQ_IDX_MAX)) {
			THM_INNER_LOG_E ("func idx is invalid. (inner thread table)\n");
			return false;
		}

		gp_thm_reg_tbl [i] = (threadmgr_reg_tbl_t*)p_tbl;
		p_tbl ++;
		++ i;
	}

	// set inner total
	set_total_worker_thread_num (i);
	THM_INNER_FORCE_LOG_I ("g_nr_total_worker_thread=[%d]\n", get_total_worker_thread_num());

	return true;
}

/**
 * dump_inner_info
 */
static void dump_inner_info (void)
{
	uint8_t i = 0;
	uint8_t j = 0;

//TODO 参照だけ ログだけだからmutexしない

	THM_LOG_I ("####  dump_inner_info  ####\n");
	THM_LOG_I (" thread-idx  thread-name  pthread_id  que-max  seq-num  req-opt  req-opt-timeout[ms]\n");

	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		THM_LOG_I (
			" 0x%02x       [%-15s] %lu %d      %d       0x%08x %d\n",
			g_inner_info [i].thread_idx,
			g_inner_info [i].name,
			g_inner_info [i].pthread_id,
			g_inner_info [i].nr_que_worker,
			g_inner_info [i].nr_seq,
			g_inner_info [i].request_option,
			g_inner_info [i].requetimeout_msec
		);
	}

	THM_LOG_I ("####  dump_seq_info  ####\n");
	THM_LOG_I (" seq-idx  seq-name  sect-id  action  is_overwrite  is_lock  timeout-state  timeout[ms]\n");
	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		THM_LOG_I (" --- thread:[0x%02x][%s]\n", g_inner_info [i].thread_idx, g_inner_info [i].name);
		int n = g_inner_info [i].nr_seq;
		seq_info_t *p_seq_info = g_inner_info [i].seq_info;
		for (j = 0; j < n; ++ j) {
			const threadmgr_seq_t *p = gp_thm_reg_tbl [g_inner_info [i].thread_idx]->seq_array;
			const char *p_name = (p + p_seq_info->seq_idx)->name;

			THM_LOG_I (
				"   0x%02x [%-15.15s] %2d%s %s %s %s %s %d\n",
				p_seq_info->seq_idx,
				p_name,
				p_seq_info->sect_id,
				p_seq_info->sect_id == p_seq_info->running_sect_id ? "<-" : "  ",
				gpsz_act [p_seq_info->en_act],
				p_seq_info->is_overwrite ? "OW" : "--",
				p_seq_info->is_lock ? "lock" : "----",
				gpsz_timeout_state [p_seq_info->timeout.en_state],
				p_seq_info->timeout.val
			);
			++ p_seq_info;
		}
	}
}

/**
 * clear_inner_info
 */
static void clear_inner_info (inner_info_t *p)
{
	if (p) {
		p->thread_idx = THREAD_IDX_BLANK;
		p->name = NULL;
		p->pthread_id = 0L;
		p->tid = 0;
		p->nr_que_worker = 0;
		p->que_worker = NULL;
		p->nr_seq = 0;
		p->seq_info = NULL;

		p->en_state = EN_STATE_INIT;
		clear_que_worker (&(p->now_exec_que_worker));

		p->request_option = 0;
		p->requetimeout_msec = 0;
	}
}

/**
 * get_state
 */
static EN_STATE get_state (uint8_t thread_idx)
{
	return g_inner_info [thread_idx].en_state;
}

/**
 * set_state
 */
static void set_state (uint8_t thread_idx, EN_STATE en_state)
{
	g_inner_info [thread_idx].en_state = en_state;
}

/**
 * set_thread_name
 * thread名をセット  pthread_setname_np
 */
void set_thread_name (char *p)
{
	if (!p) {
		return;
	}

	char sz_name [THREAD_NAME_SIZE];
	memset (sz_name, 0x00, sizeof(sz_name));
	strncpy (sz_name, p, sizeof(sz_name) -1);
	pthread_setname_np (pthread_self(), sz_name);
}

/**
 * get_task_id
 * getting kernel taskid
 */
static pid_t get_task_id (void)
{
	return syscall(SYS_gettid);
}

/**
 * is_exist_thread
 * check exist thread by kernel taskid
 */
static bool is_exist_thread (pid_t tid)
{
	char path[PATH_MAX] = {0};
	sprintf(path, "/proc/%d/task/%d", getpid(), tid);
	struct stat st;
	int ret = stat (path, &st);
	if (ret == 0) {
		return true;
	} else {
		return false;
	}
}

/**
 * check_worker_thread
 * worker_threadの確認
 */
static void check_worker_thread (void)
{
	uint8_t i = 0;
//	int n_rtn = 0;

	for (i = 0; i < get_total_worker_thread_num(); ++ i) {

		/* フラグ値を確認 */
		if ((get_state (i) != EN_STATE_READY) && (get_state (i) != EN_STATE_BUSY) && (get_state (i) != EN_STATE_WAIT_REPLY)) {
			/* 異常 */
			//TODO とりあえずログをだす
			THM_INNER_LOG_E ("[%s] EN_STATE flag is abnromal !!!\n", g_inner_info [i].name);
		}

//		n_rtn = pthread_kill (g_inner_info [i].n_pthread_id, 0);
//		if (n_rtn != SYS_RETURN_NORMAL) {
//			/* 異常 */
//			//TODO とりあえずログをだす
//			THM_INNER_LOG_E ("[%s] pthread_kill(0) abnromal !!!\n", g_inner_info [i].name);
//		}
		if (!is_exist_thread(g_inner_info [i].tid)) {
			/* 異常 */
			//TODO とりあえずログをだす
			THM_INNER_LOG_E ("[%s] thread abnromal !!!\n", g_inner_info [i].name);
		}
	}
}

/**
 * create_base_thread
 * base_thread生成
 */
static bool create_base_thread (void)
{
	pthread_t n_pthread_id;
	pthread_attr_t attr;

	if (pthread_attr_init (&attr) != 0) {
		THM_PERROR ("pthread_attr_init()");
		return false;
	}

//TODO スタックサイズ/スタックガード設定するならここで
//pthread_attr_setstacksize
//pthread_attr_setguardsize

//TODO スケジューリングポリシー設定するならここで
//ポリシー に指定できる値は SCHED_FIFO, SCHED_RR, SCHED_OTHER
//pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED) これを事前にセットして
//pthread_attr_setschedpolicy

//TODO プライオリティ設定 ポリシーRRとFIFOだったら設定
//pthread_attr_setschedparam

	if (pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED) != 0) {
		THM_PERROR ("pthread_attr_setdetachstate()");
		return false;
	}

	if (pthread_create (&n_pthread_id, &attr, base_thread, (void*)NULL) != 0) {
		THM_PERROR ("pthread_create()");
		return false;
	}

//TODO 暫定位置
	if (pthread_create (&n_pthread_id, &attr, sigwait_thread, (void*)NULL) != 0) {
		THM_PERROR ("pthread_create()");
		return false;
	}

	return true;
}

/**
 * create_worker_thread
 * worker_thread生成
 */
static bool create_worker_thread (uint8_t thread_idx)
{
	pthread_t n_pthread_id;
	pthread_attr_t attr;


	/* set thread idx */
	g_inner_info [thread_idx].thread_idx = thread_idx;


	if (pthread_attr_init( &attr ) != 0) {
		THM_PERROR ("pthread_attr_init()");
		return false;
	}

//TODO スタックサイズ/スタックガード設定するならここで
//pthread_attr_setstacksize
//pthread_attr_setguardsize

//TODO スケジューリングポリシー設定するならここで
//ポリシー に指定できる値は SCHED_FIFO, SCHED_RR, SCHED_OTHER
//pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED) これを事前にセットして
//pthread_attr_setschedpolicy

//TODO プライオリティ設定 ポリシーRRとFIFOだったら設定
//pthread_attr_setschedparam

	if (pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED) != 0) {
		THM_PERROR ("pthread_attr_setdetachstate()");
		return false;
	}

	if (pthread_create (&n_pthread_id, &attr, worker_thread, (void*)&g_inner_info [thread_idx]) != 0) {
		THM_PERROR ("pthread_create()");
		return false;
	}

	return true;
}

/**
 * create_all_thread
 * 全スレッド生成
 */
static bool create_all_thread (void)
{
	uint8_t i = 0;

	if (!create_base_thread()) {
		return false;
	}

	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		if (!create_worker_thread (i)) {
			return false;
		}
		THM_INNER_FORCE_LOG_I ("create worker_thread. th_idx:[%d]\n", i);
	}

	return true;
}

/**
 * enqueue (base)
 */
static bool enque_base (EN_MONI_TYPE en_type)
{
	int i = 0;

	pthread_mutex_lock (&g_mutex_ope_que_base);

	for (i = 0; i < QUE_BASE_NUM; ++ i) {

		/* 空きを探す */
		if (g_que_base [i].is_used == false) {
			g_que_base [i].en_moni_type = en_type;
			g_que_base [i].is_used = true;
			break;
		}
	}

	pthread_mutex_unlock (&g_mutex_ope_que_base);

	if (i == QUE_BASE_NUM) {
		/* 全部埋まってる */
		THM_INNER_LOG_E ("que is full. (base)\n");
		return false;
	}

	return true;
}

/**
 * dequeue (base)
 */
static que_base_t deque_base (bool is_get_out)
{
	int i = 0;
	que_base_t rtn;

//TODO いらないかも ここはbase_threadで処理するから
	pthread_mutex_lock (&g_mutex_ope_que_base);

	rtn = g_que_base [0];

	if (is_get_out) {
		for (i = 0; i < QUE_BASE_NUM; ++ i) {
			if (i < QUE_BASE_NUM-1) {
				memcpy (&g_que_base [i], &g_que_base [i+1], sizeof(que_base_t));

			} else {
				/* 末尾 */
				g_que_base [i].en_moni_type = EN_MONI_TYPE_INIT;
				g_que_base [i].is_used = false;
			}
		}
	}

	pthread_mutex_unlock (&g_mutex_ope_que_base);

	return rtn;
}

/**
 * baseキュークリア
 */
static void clear_que_base (que_base_t *p)
{
	if (!p) {
		return;
	}

	p->en_moni_type = EN_MONI_TYPE_INIT;
	p->is_used = false;
}

/**
 * enqueue (worker)
 * 引数 thread_idx,seq_idxは宛先
 */
static bool enque_worker (
	uint8_t thread_idx,
	uint8_t seq_idx,
	EN_QUE_TYPE en_que_type,
	context_t *context,
	uint32_t req_id,
	EN_THM_RSLT en_rslt,
	uint8_t client_id,
	uint8_t *msg,
	size_t msg_size
)
{
	uint8_t i = 0;
	uint8_t nr_que_worker = g_inner_info [thread_idx].nr_que_worker;
	que_worker_t *p_que_worker = g_inner_info [thread_idx].que_worker;

	/* 複数の人が同じthread_idxのキューを操作するのをガード */
	pthread_mutex_lock (&g_mutex_ope_que_worker [thread_idx]);

	for (i = 0; i < nr_que_worker; ++ i) {

		/* 空きを探す */
		if (p_que_worker->is_used == false) {

			/* set */
			p_que_worker->dest_thread_idx = thread_idx;
			p_que_worker->dest_seq_idx = seq_idx;
			if (context) {
				p_que_worker->is_valid_src_info = context->is_valid;
				p_que_worker->src_thread_idx = context->thread_idx;
				p_que_worker->src_seq_idx = context->seq_idx;
			}
			p_que_worker->en_que_type = en_que_type;
			p_que_worker->req_id = req_id;
			p_que_worker->en_rslt = en_rslt;
			p_que_worker->client_id = client_id;
			if (msg && msg_size > 0) {
				if (msg_size > MSG_SIZE) {
					THM_INNER_FORCE_LOG_W ("truncate request message. size:[%lu]->[%d] th_idx:[%d]\n", msg_size, MSG_SIZE, thread_idx);
				}
				memset (p_que_worker->msg.msg, 0x00, MSG_SIZE);
				memcpy (p_que_worker->msg.msg, msg, msg_size < MSG_SIZE ? msg_size : MSG_SIZE);
				p_que_worker->msg.size = msg_size < MSG_SIZE ? msg_size : MSG_SIZE;
				p_que_worker->msg.is_used = true;
			}
			p_que_worker->is_used = true;
			break;
		}

		p_que_worker ++;
	}

#if 1
	if (i < nr_que_worker) {
		p_que_worker = g_inner_info [thread_idx].que_worker;
		uint8_t j = 0;
		for (j = 0; j <= i; ++ j) {
			THM_INNER_LOG_I (
				" %d: %s (%s %d-%d) -> %d-%d 0x%x %s 0x%x %s\n",
				j,
				gpsz_que_type [p_que_worker->en_que_type],
				p_que_worker->is_valid_src_info ? "T" : "F",
				p_que_worker->n_src_thread_idx,
				p_que_worker->n_src_seq_idx,
				p_que_worker->n_dest_thread_idx,
				p_que_worker->n_dest_seq_idx,
				p_que_worker->req_id,
				gpsz_rslt [p_que_worker->en_rslt],
				p_que_worker->client_id,
				p_que_worker->is_used ? "T" : "F"
			);
			p_que_worker ++;
		}
	}
#else
	/* dump */
	p_que_worker = g_inner_info [thread_idx].p_que_worker;
	THM_INNER_LOG_I( "####  en_que dest[%s]  ####\n", g_inner_info [thread_idx].name );
	uint8_t j = 0;
	for (j = 0; j < nr_que_worker; ++ j) {
		THM_INNER_LOG_I (
			" %d: %s (%s %d-%d) -> %d-%d 0x%x %s 0x%x %s\n",
			j,
			gpsz_que_type [p_que_worker->en_que_type],
			p_que_worker->is_valid_src_info ? "T" : "F",
			p_que_worker->n_src_thread_idx,
			p_que_worker->n_src_seq_idx,
			p_que_worker->n_dest_thread_idx,
			p_que_worker->n_dest_seq_idx,
			p_que_worker->req_id,
			gpsz_rslt [p_que_worker->en_rslt],
			p_que_worker->client_id,
			p_que_worker->is_used ? "T" : "F"
		);
		p_que_worker ++;
	}
#endif

	pthread_mutex_unlock (&g_mutex_ope_que_worker [thread_idx]);

	if (i == nr_que_worker) {
		/* 全部埋まってる */
		THM_INNER_LOG_E ("que is full. (worker) th_idx:[%d]\n", thread_idx);
		dump_que_worker (thread_idx);
		return false;
	}

	return true;
}

/**
 * dump_que_worker
 */
static void dump_que_worker (uint8_t thread_idx)
{
	uint8_t i = 0;
	uint8_t nr_que_worker = g_inner_info [thread_idx].nr_que_worker;
	que_worker_t *p_que_worker = g_inner_info [thread_idx].que_worker;

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_que_worker [thread_idx]);

	THM_LOG_I ("####  dump_que [%s]  ####\n", g_inner_info [thread_idx].name);
	for (i = 0; i < nr_que_worker; ++ i) {
		if (!p_que_worker->is_used) {
			continue;
		}

		THM_LOG_I (
			" %d: %s (%s %d-%d) -> %d-%d 0x%x %s 0x%x %s\n",
			i,
			gpsz_que_type [p_que_worker->en_que_type],
			p_que_worker->is_valid_src_info ? "vaild  " : "invalid",
			p_que_worker->src_thread_idx,
			p_que_worker->src_seq_idx,
			p_que_worker->dest_thread_idx,
			p_que_worker->dest_seq_idx,
			p_que_worker->req_id,
			gpsz_rslt [p_que_worker->en_rslt],
			p_que_worker->client_id,
			p_que_worker->is_used ? "used  " : "unused"
		);
		p_que_worker ++;
	}

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_que_worker [thread_idx]);
}

/**
 * dump_que_all_thread
 */
static void dump_que_all_thread (void)
{
	uint8_t i = 0;
	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		dump_que_worker (i);
	}
}

#if 0
/**
 * dequeue (worker)
 */
static que_worker_t deque_worker (uint8_t thread_idx, bool is_get_out)
{
	uint8_t i = 0;
	uint8_t nr_que_worker = g_inner_info [thread_idx].nr_que_worker;
	que_worker_t *p_que_worker = g_inner_info [thread_idx].p_que_worker;
	que_worker_t rtn;

	clear_que_worker (&rtn);

	/* 複数の人が同じthread_idxのキューを操作するのをガード */
	pthread_mutex_lock (&g_mutex_ope_que_worker [thread_idx]);

	memcpy (&rtn, p_que_worker, sizeof (que_worker_t));

	if (is_get_out) {
		for (i = 0; i < nr_que_worker; ++ i) {
			if (i < nr_que_worker-1) {
				memcpy (
					p_que_worker,
					p_que_worker +1,
					sizeof (que_worker_t)
				);

			} else {
				/* 末尾 */
				clear_que_worker (p_que_worker);
			}

			p_que_worker ++;
		}
	}

	pthread_mutex_unlock (&g_mutex_ope_que_worker [thread_idx]);

	return rtn;
}
#endif

/**
 * dequeue (worker)
 * 実行できるキューの検索込のdequeue
 */
//TODO ここではis_get_out無くし チェックのみで dequeする関数は別にした方がいいのかもしれない
static que_worker_t check2deque_worker (uint8_t thread_idx, bool is_get_out)
{
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t k = 0;
	uint8_t l = 0;
	uint8_t nr_que_worker = g_inner_info [thread_idx].nr_que_worker;
	que_worker_t *p_que_worker = g_inner_info [thread_idx].que_worker;
	que_worker_t rtn;
	uint8_t seq_idx = 0;
	EN_TIMEOUT_STATE en_timeout_state = EN_TIMEOUT_STATE_INIT;

	clear_que_worker (&rtn);


	/* 複数の人が同じthread_idxのキューを操作するのをガード */
	pthread_mutex_lock (&g_mutex_ope_que_worker [thread_idx]);

	/* 新しいほうからキューを探す */
	for (i = 0; i < nr_que_worker; ++ i) {

		if (p_que_worker->is_used) {

			if (p_que_worker->en_que_type != EN_QUE_TYPE_NOTIFY) {
				/* lockはNOTIFYは除外します */

				if (is_lock (thread_idx)) {
					/* 
					 * だれかlockしている
					 * (当該Threadのseqのどれかでlockしている)
					 */
					seq_idx = p_que_worker->dest_seq_idx;
					if (!is_lock_seq (thread_idx, seq_idx)) {
						/* 対象のseq以外がlockしていたら 見送ります */
						p_que_worker ++;
						continue;
					}
				}
			}


			if (p_que_worker->en_que_type == EN_QUE_TYPE_REQUEST) {
				/* * -------------------- REQUEST_QUE -------------------- * */ 

				seq_idx = p_que_worker->dest_seq_idx;
				if (get_seq_info (thread_idx, seq_idx)->is_overwrite) {
					/*
					 * overwrite有効中
					 * 強制実行します
					 */
					memcpy (&rtn, p_que_worker, sizeof(que_worker_t));

//TODO この位置はよくないか
					/* 取り出す時だけ */
					if (is_get_out) {
						/* sectid関係を強制クリア */
						clear_sect_id (thread_idx, seq_idx);
						/* Seqタイムアウトを強制クリア */
						clear_seq_timeout (thread_idx, seq_idx);
					}

					break;

				} else {
					/* overwrite無効 */

					if (get_seq_info (thread_idx, seq_idx)->en_act == EN_THM_ACT_INIT) {
						/*
						 * 対象のシーケンスがEN_THM_ACT_INIT
						 * 実行してok
						 */
						memcpy (&rtn, p_que_worker, sizeof(que_worker_t));
						break;

					} else if (get_seq_info (thread_idx, seq_idx)->en_act == EN_THM_ACT_WAIT) {
						/*
						 * 対象のシーケンスがEN_THM_ACT_WAIT シーケンスの途中
						 * 見送り
						 */
						THM_INNER_LOG_I (
							"en_act is EN_THM_ACT_WAIT @REQUEST_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x]) ---> through\n",
							gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->name,
							p_que_worker->n_src_thread_idx,
							gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->seq_array[p_que_worker->n_src_seq_idx].name,
							p_que_worker->n_src_seq_idx,
							gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->name,
							p_que_worker->n_dest_thread_idx,
							gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->seq_array[p_que_worker->n_dest_seq_idx].name,
							p_que_worker->n_dest_seq_idx,
							p_que_worker->req_id
						);

					} else {
						/* ありえない */
						THM_INNER_LOG_E (
							"Unexpected: en_act is [%d] @REQUEST_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x])\n",
							get_seq_info(thread_idx, seq_idx)->en_act,
							gp_thm_reg_tbl [p_que_worker->src_thread_idx]->name,
							p_que_worker->src_thread_idx,
							gp_thm_reg_tbl [p_que_worker->src_thread_idx]->seq_array[p_que_worker->src_seq_idx].name,
							p_que_worker->src_seq_idx,
							gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->name,
							p_que_worker->dest_thread_idx,
							gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->seq_array[p_que_worker->dest_seq_idx].name,
							p_que_worker->dest_seq_idx,
							p_que_worker->req_id
						);
					}
				}



			} else if (p_que_worker->en_que_type == EN_QUE_TYPE_REPLY) {
				/* * -------------------- REPLY_QUE -------------------- * */

				seq_idx = p_que_worker->dest_seq_idx;
				if (get_seq_info (thread_idx, seq_idx)->en_act == EN_THM_ACT_INIT) {
					/* シーケンスによってはありえる */
					/* リプライ待たずに進むようなシーケンスとか... */
					THM_INNER_FORCE_LOG_I (
						"en_act is EN_THM_ACT_INIT @REPLY_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x])  ---> drop\n",
						gp_thm_reg_tbl [p_que_worker->src_thread_idx]->name,
						p_que_worker->src_thread_idx,
						gp_thm_reg_tbl [p_que_worker->src_thread_idx]->seq_array[p_que_worker->src_seq_idx].name,
						p_que_worker->src_seq_idx,
						gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->name,
						p_que_worker->dest_thread_idx,
						gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->seq_array[p_que_worker->dest_seq_idx].name,
						p_que_worker->dest_seq_idx,
						p_que_worker->req_id
					);

					/* この場合キューは引き取る */
					p_que_worker->is_drop = true;
					release_request_id (thread_idx, p_que_worker->req_id);

				} else if (get_seq_info (thread_idx, seq_idx)->en_act == EN_THM_ACT_WAIT) {
					/*
					 * 対象のシーケンスがEN_THM_ACT_WAIT シーケンスの途中
					 * request_idを確認する
					 */
#ifndef _MULTI_REQUESTING
					if (p_que_worker->req_id == get_seq_info (thread_idx,seq_idx)->req_id) {
#else
					/*
					 * 複数requestの場合があるので request_id_infoで照合する
					 * request_id_infoで照合するといことは当スレッドの全リクエストが対象になります
					 * ただし過去に複数リクエストしている場合があるので 今待ち受けたリプライが本物かどうかはユーザがわでreq_idの判定が必要
					 */
					if (p_que_worker->req_id == get_request_id_info (thread_idx, p_que_worker->req_id)->id) {
#endif
						/*
						 * request_idが一致
						 * 実行してok
						 */
						memcpy (&rtn, p_que_worker, sizeof(que_worker_t));
						if (is_get_out) {
							/* dequeするときにrelease */
							release_request_id (thread_idx, p_que_worker->req_id);
						}
						break;

					} else {
						/* シーケンスによってはありえる */
						/* リプライ待たずに進むようなシーケンスとか... */
#ifndef _MULTI_REQUESTING
						THM_INNER_LOG_I (
							"en_act is EN_THM_ACT_WAIT  req_id unmatch:[%d:%d] @REPLY_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x])  ---> drop\n",
							p_que_worker->req_id,
							get_seq_info (thread_idx, seq_idx)->req_id,
							gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->name,
							p_que_worker->n_src_thread_idx,
							gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->seq_array[p_que_worker->n_src_seq_idx].name,
							p_que_worker->n_src_seq_idx,
							gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->name,
							p_que_worker->n_dest_thread_idx,
							gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->seq_array[p_que_worker->n_dest_seq_idx].name,
							p_que_worker->n_dest_seq_idx,
							p_que_worker->req_id
						);
#else
						THM_INNER_LOG_I (
							"en_act is EN_THM_ACT_WAIT  req_id unmatch @REPLY_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x])  ---> drop\n",
							gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->name,
							p_que_worker->n_src_thread_idx,
							gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->seq_array[p_que_worker->n_src_seq_idx].name,
							p_que_worker->n_src_seq_idx,
							gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->name,
							p_que_worker->n_dest_thread_idx,
							gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->seq_array[p_que_worker->n_dest_seq_idx].name,
							p_que_worker->n_dest_seq_idx,
							p_que_worker->req_id
						);
#endif
						/* この場合キューは引き取る */
						p_que_worker->is_drop = true;
						release_request_id (thread_idx, p_que_worker->req_id);
					}

				} else {
					/* ありえない */
					THM_INNER_LOG_E (
						"Unexpected: en_act is [%d] @REPLY_QUE (from[%s(%d):%s](%d) to[%s(%d):%s(%d)] req_id[0x%x])\n",
						get_seq_info (thread_idx, seq_idx)->en_act,
						gp_thm_reg_tbl [p_que_worker->src_thread_idx]->name,
						p_que_worker->src_thread_idx,
						gp_thm_reg_tbl [p_que_worker->src_thread_idx]->seq_array[p_que_worker->src_seq_idx].name,
						p_que_worker->src_seq_idx,
						gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->name,
						p_que_worker->dest_thread_idx,
						gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->seq_array[p_que_worker->dest_seq_idx].name,
						p_que_worker->dest_seq_idx,
						p_que_worker->req_id
					);
				}



			} else if (p_que_worker->en_que_type == EN_QUE_TYPE_NOTIFY) {
				/* * -------------------- NOTIFY_QUE -------------------- * */

				/* そのまま実行してok */
				memcpy (&rtn, p_que_worker, sizeof(que_worker_t));
				break;



			} else if (p_que_worker->en_que_type == EN_QUE_TYPE_REQ_TIMEOUT) {
				/* * -------------------- REQ_TIMEOUT_QUE -------------------- * */

				/* req_idのタイムアウト状態を確認する */
				en_timeout_state = get_req_timeout_state (thread_idx, p_que_worker->req_id);
				if (en_timeout_state == EN_TIMEOUT_STATE_PASSED) {

					seq_idx = p_que_worker->dest_seq_idx;
					if (get_seq_info (thread_idx, seq_idx)->en_act == EN_THM_ACT_WAIT) {
						/*
						 * 対象のシーケンスがEN_THM_ACT_WAIT
						 * req_idの一致を確認する
						 */
#ifndef _MULTI_REQUESTING
						if (p_que_worker->req_id == get_seq_info (thread_idx, seq_idx)->req_id) {
#else
						/*
						 * 複数requestの場合があるので request_id_infoで照合する
						 * request_id_infoで照合するといことは当スレッドの全リクエストが対象になります
						 * ただし過去に複数リクエストしている場合があるので 今待ち受けたリプライが本物かどうかはユーザがわでreq_idの判定が必要
						 */
						if (p_que_worker->req_id == get_request_id_info (thread_idx, p_que_worker->req_id)->id) {
#endif
							/*
							 * req_idが一致した
							 * 実行してok
							 */
							memcpy (&rtn, p_que_worker, sizeof(que_worker_t));
							if (is_get_out) {
								/* dequeするときにrelease */
								release_request_id (thread_idx, p_que_worker->req_id);
							}
							break;

						} else {
							/* シーケンスによってはありえる */
							/* リプライ待たずに進むようなシーケンスとか... */
#ifndef _MULTI_REQUESTING
							THM_INNER_LOG_I (
								"en_act is EN_THM_ACT_WAIT  req_id unmatch:[%d:%d] @REQ_TIMEOUT_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x])  ---> drop\n",
								p_que_worker->req_id,
								get_seq_info (thread_idx, seq_idx)->req_id
								gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->name,
								p_que_worker->n_src_thread_idx,
								gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->seq_array[p_que_worker->n_src_seq_idx].name,
								p_que_worker->n_src_seq_idx,
								gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->name,
								p_que_worker->n_dest_thread_idx,
								gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->seq_array[p_que_worker->n_dest_seq_idx].name,
								p_que_worker->n_dest_seq_idx,
								p_que_worker->req_id
							);
#else
							THM_INNER_LOG_I (
								"en_act is EN_THM_ACT_WAIT  req_id notfound @REQ_TIMEOUT_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x])  ---> drop\n",
								gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->name,
								p_que_worker->n_src_thread_idx,
								gp_thm_reg_tbl [p_que_worker->n_src_thread_idx]->seq_array[p_que_worker->n_src_seq_idx].name,
								p_que_worker->n_src_seq_idx,
								gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->name,
								p_que_worker->n_dest_thread_idx,
								gp_thm_reg_tbl [p_que_worker->n_dest_thread_idx]->seq_array[p_que_worker->n_dest_seq_idx].name,
								p_que_worker->n_dest_seq_idx,
								p_que_worker->req_id
							);
#endif

							/* この場合キューは引き取る */
							p_que_worker->is_drop = true;
							release_request_id (thread_idx, p_que_worker->req_id);
						}

					} else {
						/* シーケンスによってはありえる */
						/* リプライ待たずに進むようなシーケンスとか... */
						THM_INNER_FORCE_LOG_I (
							"en_act is not EN_THM_ACT_WAIT  @REQ_TIMEOUT_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x])  ---> drop\n",
							gp_thm_reg_tbl [p_que_worker->src_thread_idx]->name,
							p_que_worker->src_thread_idx,
							gp_thm_reg_tbl [p_que_worker->src_thread_idx]->seq_array[p_que_worker->src_seq_idx].name,
							p_que_worker->src_seq_idx,
							gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->name,
							p_que_worker->dest_thread_idx,
							gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->seq_array[p_que_worker->dest_seq_idx].name,
							p_que_worker->dest_seq_idx,
							p_que_worker->req_id
						);

						/* この場合キューは引き取る */
						p_que_worker->is_drop = true;
						release_request_id (thread_idx, p_que_worker->req_id);
					}

				} else {
					/* ありえないはず */
					THM_INNER_LOG_E (
						"Unexpected: req_id_info.timeout.en_state is unexpected [%d] @REQ_TIMEOUT_QUE (from[%s(%d):%s(%d)] to[%s(%d):%s(%d)] req_id[0x%x])\n",
						en_timeout_state,
						gp_thm_reg_tbl [p_que_worker->src_thread_idx]->name,
						p_que_worker->src_thread_idx,
						gp_thm_reg_tbl [p_que_worker->src_thread_idx]->seq_array[p_que_worker->src_seq_idx].name,
						p_que_worker->src_seq_idx,
						gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->name,
						p_que_worker->dest_thread_idx,
						gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->seq_array[p_que_worker->dest_seq_idx].name,
						p_que_worker->dest_seq_idx,
						p_que_worker->req_id
					);
				}



			} else if (p_que_worker->en_que_type == EN_QUE_TYPE_SEQ_TIMEOUT) {
				/* * -------------------- SEQ_TIMEOUT_QUE -------------------- * */

				seq_idx = p_que_worker->dest_seq_idx;
				if (get_seq_info (thread_idx, seq_idx)->en_act == EN_THM_ACT_WAIT) {
					/*
					 * 対象のシーケンスがEN_THM_ACT_TIMEOUT
					 * seq_infoのタイムアウト状態を確認する
					 */
					if (get_seq_info (thread_idx, seq_idx)->timeout.en_state == EN_TIMEOUT_STATE_PASSED) {
						/*
						 * 既にタイムアウトしている
						 * 実行してok
						 */
						memcpy (&rtn, p_que_worker, sizeof(que_worker_t));
						break;

					} else {
						/* ありえるのか? ありえないはず... */
						THM_INNER_LOG_E (
							"Unexpected: en_act is EN_THM_ACT_WAIT  unexpect timeout.en_state:[%d]  @SEQ_TIMEOUT_QUE (to[%s(%d):%s(%d)] req_id[0x%x])\n",
							get_seq_info(thread_idx, seq_idx)->timeout.en_state,
							gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->name,
							p_que_worker->dest_thread_idx,
							gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->seq_array[p_que_worker->dest_seq_idx].name,
							p_que_worker->dest_seq_idx,
							p_que_worker->req_id
						);
					}

				} else {
					/* ありえないはず */
					THM_INNER_LOG_E (
						"Unexpected: en_act is [%d] @SEQ_TIMEOUT_QUE (to[%s(%d):%s(%d)] req_id[0x%x])\n",
						get_seq_info (thread_idx, seq_idx)->en_act,
						gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->name,
						p_que_worker->dest_thread_idx,
						gp_thm_reg_tbl [p_que_worker->dest_thread_idx]->seq_array[p_que_worker->dest_seq_idx].name,
						p_que_worker->dest_seq_idx,
						p_que_worker->req_id
					);
				}



			} else if (p_que_worker->en_que_type == EN_QUE_TYPE_DESTROY) {
				/* * -------------------- DESTROY_QUE -------------------- * */

				/* そのまま実行してok */
				memcpy (&rtn, p_que_worker, sizeof(que_worker_t));
				break;



			} else {
				THM_INNER_LOG_E ("Unexpected: unexpected que_type.\n");
			}
		} else {
//THM_INNER_LOG_E( "not use\n" );
		}

		p_que_worker ++;

	} // for (i = 0; i < nr_que_worker; ++ i) {


	if (i == nr_que_worker) {
		/* not found */
		THM_INNER_LOG_D ("not found. th_idx:[%d]\n", thread_idx);

	} else {
		if (is_get_out) {
			/* 見つかったので並び換える */
			p_que_worker = g_inner_info [thread_idx].que_worker;
			for (j = 0; j < nr_que_worker; ++ j) {
				if (j >= i) {
					if (j < nr_que_worker -1) {
						memcpy (
							p_que_worker +j,
							p_que_worker +j +1,
							sizeof (que_worker_t)
						);

					} else {
						/* 末尾 */
						clear_que_worker (p_que_worker +j);
					}
				}
			}
		}
	}


	/* is_dropフラグ立っているものは消します  逆からみます */
	/* is_dropの削除はis_get_out関係なく処理する */
	p_que_worker = g_inner_info [thread_idx].que_worker;
	for (k = nr_que_worker -1; (k >= 0) && (k < nr_que_worker); k --) { // unsigned minus value
		if ((p_que_worker +k)->is_drop) {
			if (k < nr_que_worker -1) {
				for (l = k; l < nr_que_worker -1; l ++) {
					memcpy (
						p_que_worker +l,
						p_que_worker +l +1,
						sizeof (que_worker_t)
					);
				}

			} else {
				/* 末尾 */
				clear_que_worker (p_que_worker +k);
			}
		}
	}

	pthread_mutex_unlock (&g_mutex_ope_que_worker [thread_idx]);

	return rtn;
}

/**
 * clear_que_worker
 * workerキュークリア
 */
static void clear_que_worker (que_worker_t *p)
{
	if (!p) {
		return;
	}

	p->dest_thread_idx = THREAD_IDX_BLANK;
	p->dest_seq_idx = SEQ_IDX_BLANK;
	p->is_valid_src_info = false;
	p->src_thread_idx = THREAD_IDX_BLANK;
	p->src_seq_idx = SEQ_IDX_BLANK;
	p->en_que_type = EN_QUE_TYPE_INIT;
	p->req_id = REQUEST_ID_BLANK;
	p->en_rslt = EN_THM_RSLT_IGNORE;
	p->client_id = NOTIFY_CLIENT_ID_BLANK;
	memset (p->msg.msg, 0x00, MSG_SIZE);
	p->msg.size = 0;
	p->msg.is_used = false;
	p->is_used = false;
	p->is_drop = false;
}

/**
 * base_thread -- main loop
 */
static void *base_thread (void *arg)
{
	int n_rtn = 0;
	bool is_destroy = false;
	que_base_t rtn_que;
	struct timespec timeout = {0};
	struct timeval now_timeval = {0};


	g_tid_base_thread = get_task_id ();

	/* set thread name */
	set_thread_name (BASE_THREAD_NAME);

	int policy;
	struct sched_param param;
	if (pthread_getschedparam (pthread_self(), &policy, &param) != 0) {
		THM_PERROR ("pthread_getschedparam()");
	}

	THM_INNER_FORCE_LOG_I (
		"----- %s created. ----- (pthread_id:[%lu] policy:[%s] priority:[%d])\n",
		BASE_THREAD_NAME,
		pthread_self(),
		policy == SCHED_FIFO ? "SCHED_FIFO" :
			policy == SCHED_RR ? "SCHED_RR" :
				policy == SCHED_OTHER ? "SCHED_OTHER" :
					"???",
		param.sched_priority
	);

	/* スレッド立ち上がり通知 */
	post_sem ();


	while (1) {

		/* lock */
		pthread_mutex_lock (&g_mutex_base);

		/*
		 * キューに入っているかチェック
		 * なければキューを待つ
		 */
		if (!deque_base(false).is_used) {

			get_time_of_day (&now_timeval);
			timeout.tv_sec = now_timeval.tv_sec + BASE_THREAD_LOOP_TIMEOUT_SEC;
			timeout.tv_nsec = now_timeval.tv_usec * 1000;

			n_rtn = pthread_cond_timedwait (&g_cond_base, &g_mutex_base, &timeout);
			switch (n_rtn) {
			case SYS_RETURN_NORMAL:
				/* タイムアウト前に動き出した */
				break;

			case ETIMEDOUT:
				/*
				 * タイムアウトした
				 */
				THM_INNER_LOG_D ("ETIMEDOUT base_thread\n");

				/* 外部スレッドのReqタイムアウトチェック */
				check_req_timeout (THREAD_IDX_EXTERNAL);

				/* workerスレッドの確認 */
				check_worker_thread();

				break;

			case EINTR:
				/* シグナルに割り込まれた */
				THM_INNER_LOG_W ("interrupt occured.\n");
				break;

			default:
				THM_INNER_LOG_E ("Unexpected: pthread_cond_timedwait() => unexpected return value [%d]\n", n_rtn);
				break;
			}

		}

		/* unlock */
		pthread_mutex_unlock (&g_mutex_base);


		/*
		 * キューから取り出す
		 */
		rtn_que = deque_base (true);
		if (rtn_que.is_used) {
			switch (rtn_que.en_moni_type) {
			case EN_MONI_TYPE_DEUnexpected:
				dump_inner_info ();
				dump_request_id_info ();
				dump_ext_info_list ();
				dump_que_all_thread ();
				dump_notify_client_info ();

				{
					int i = 0;
					for (i = 0; i < get_total_worker_thread_num(); ++ i) {
						if (i == get_total_worker_thread_num() -1) {
							// last
							pthread_kill (g_inner_info[i].pthread_id, SIGRTMIN +2);
						} else {
							pthread_kill (g_inner_info[i].pthread_id, SIGRTMIN +1);
						}
						usleep(500000);
					}
				}

				break;

			case EN_MONI_TYPE_DESTROY:
				is_destroy = true;
				break;

			default:
				break;
			}
		}

		if (is_destroy) {
			break;
		}
	}


	THM_INNER_FORCE_LOG_I ("----- %s destryed. -----\n", BASE_THREAD_NAME);
	return NULL;
}

/**
 * sigwait_thread -- main loop
 */
static void *sigwait_thread (void *arg)
{
	int n_sig = 0;
	bool is_destroy = false;


	g_tid_sig_wait_thread = get_task_id ();

	/* set thread name */
	set_thread_name (SIGWAIT_THREAD_NAME);

	int policy;
	struct sched_param param;
	if (pthread_getschedparam (pthread_self(), &policy, &param) != 0) {
		THM_PERROR ("pthread_getschedparam()");
	}

	THM_INNER_FORCE_LOG_I (
		"----- %s created. ----- (pthread_id:[%lu] policy:[%s] priority:[%d])\n",
		SIGWAIT_THREAD_NAME,
		pthread_self(),
		policy == SCHED_FIFO ? "SCHED_FIFO" :
			policy == SCHED_RR ? "SCHED_RR" :
				policy == SCHED_OTHER ? "SCHED_OTHER" :
					"???",
		param.sched_priority
	);

	/* スレッド立ち上がり通知 */
	post_sem ();


	while (1) {
		if (sigwait(&g_sigset, &n_sig) == SYS_RETURN_NORMAL) {
			switch (n_sig) {
			case SIGQUIT:
				THM_INNER_FORCE_LOG_I ("catch SIGQUIT\n");
				request_base_thread (EN_MONI_TYPE_DEUnexpected);
				break;
			case SIGINT:
			case SIGTERM:
				THM_INNER_FORCE_LOG_I ("catch SIGINT or SIGTERM\n");
				request_base_thread (EN_MONI_TYPE_DESTROY);
				destroy_all_worker_thread();
				is_destroy = true;
				break;
			default:
				THM_INNER_LOG_E ("Unexpected: unexpected signal.\n");
				break;
			}
		} else {
			THM_PERROR ("sigwait()");
		}

		if (is_destroy) {
			break;
		}
	}


	THM_INNER_FORCE_LOG_I ("----- %s destroyed. -----\n", SIGWAIT_THREAD_NAME);
	return NULL;
}

/**
 * check_wait_worker_thread
 */
static void check_wait_worker_thread (inner_info_t *inner_info)
{
	int n_rtn = 0;
	EN_NEAREST_TIMEOUT en_timeout = EN_NEAREST_TIMEOUT_NONE;
	request_id_info_t *p_req_id_info = NULL;
	seq_info_t *p_seq_info = NULL;
	struct timespec *p_timeout = NULL;
	bool is_need_wait = true;

	/* lock */
	pthread_mutex_lock (&g_mutex_worker [inner_info->thread_idx]);

	/*
	 * 実行すべきキューがあるかチェック
	 * なければキューを待つ
	 */
//	if (!deque_worker( inner_info->thread_idx, false ).is_used) {
	if (!check2deque_worker (inner_info->thread_idx, false).is_used) {

		set_state (inner_info->thread_idx, EN_STATE_READY);

		en_timeout = search_nearetimeout (inner_info->thread_idx, &p_req_id_info, &p_seq_info);
		if (en_timeout == EN_NEAREST_TIMEOUT_NONE) {
			THM_INNER_LOG_D ("normal cond wait\n");

			/*
			 * 通常の cond wait
			 * pthread_cond_signal待ち
			 */
			while (is_need_wait) {
				n_rtn = pthread_cond_wait (
					&g_cond_worker [inner_info->thread_idx],
					&g_mutex_worker [inner_info->thread_idx]
				);
				switch (n_rtn) {
				case EINTR:
					THM_INNER_LOG_W ("interrupt occured.\n");
					break;
				default:
					is_need_wait = false;
					break;
				}
			}

		} else {
			/* タイムアウト仕掛かり中です */

			if (en_timeout == EN_NEAREST_TIMEOUT_REQ) {
				THM_INNER_LOG_D ("timeout cond wait (req_timeout)\n");

				if (!p_req_id_info) {
					THM_INNER_LOG_E ("Unexpected: p_req_id_info is null !!!\n");
					goto _F_END;
				}
				p_req_id_info->timeout.en_state = EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT;
				p_timeout = &(p_req_id_info->timeout.time);

			} else {
				THM_INNER_LOG_D ("timeout cond wait (seq_timeout)\n");

				if (!p_seq_info) {
					THM_INNER_LOG_E ("Unexpected: p_seq_info is null !!!\n");
					goto _F_END;
				}
				p_seq_info->timeout.en_state = EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT;
				p_timeout = &(p_seq_info->timeout.time);
			}

			/*
			 * タイムアウト付き cond wait
			 * pthread_cond_signal待ち
			 */
			while (is_need_wait) {
				n_rtn = pthread_cond_timedwait (
					&g_cond_worker [inner_info->thread_idx],
					&g_mutex_worker [inner_info->thread_idx],
					p_timeout
				);
				switch (n_rtn) {
				case SYS_RETURN_NORMAL:
					/* タイムアウト前に動き出した */
					is_need_wait = false;
					if (en_timeout == EN_NEAREST_TIMEOUT_REQ) {
						if (!p_req_id_info) {
							THM_INNER_LOG_E ("Unexpected: p_req_id_info is null !!!\n");
							goto _F_END;
						}
						p_req_id_info->timeout.en_state = EN_TIMEOUT_STATE_MEAS;
	
					} else {
						if (!p_seq_info) {
							THM_INNER_LOG_E ("Unexpected: p_seq_info is null !!!\n");
							goto _F_END;
						}
						p_seq_info->timeout.en_state = EN_TIMEOUT_STATE_MEAS;
					}
					break;
	
				case ETIMEDOUT:
					/*
					 * タイムアウトした
					 * 自スレッドにタイムアウトのキューを入れる
					 */
					is_need_wait = false;
					if (en_timeout == EN_NEAREST_TIMEOUT_REQ) {
						if (!p_req_id_info) {
							THM_INNER_LOG_E ("Unexpected: p_req_id_info is null !!!\n");
							goto _F_END;
						}
						p_req_id_info->timeout.en_state = EN_TIMEOUT_STATE_PASSED;
						enque_req_timeout (inner_info->thread_idx, p_req_id_info->id);
	
					} else {
						if (!p_seq_info) {
							THM_INNER_LOG_E ("Unexpected: p_seq_info is null !!!\n");
							goto _F_END;
						}
						p_seq_info->timeout.en_state = EN_TIMEOUT_STATE_PASSED;
						enque_seq_timeout (inner_info->thread_idx, p_seq_info->seq_idx);
					}
					break;
	
				case EINTR:
					THM_INNER_LOG_W ("interrupt occured.\n");
					break;
	
				default:
					is_need_wait = false;
					THM_INNER_LOG_E ("Unexpected: pthread_cond_timedwait() => unexpected return value [%d]\n", n_rtn);
					break;
				}
			}
		}
	}

_F_END:
	/* unlock */
	pthread_mutex_unlock (&g_mutex_worker [inner_info->thread_idx]);
}

/**
 * worker_thread -- main loop
 */
static void *worker_thread (void *arg)
{
	uint8_t i = 0;
	bool is_destroy = false;
	inner_info_t *inner_info = (inner_info_t*)arg;
	threadmgr_reg_tbl_t *p_tbl = gp_thm_reg_tbl [inner_info->thread_idx];
	que_worker_t rtn_que;
	threadmgr_src_info_t *p_thm_src_info = &g_thm_src_info [inner_info->thread_idx];


	inner_info->tid = get_task_id ();

	setup_signal_per_thread();

	/* init thm_if */
	threadmgr_if_t thm_if;
	clear_thm_if (&thm_if);


	/* --- set thread name --- */
	set_thread_name ((char*)p_tbl->name);
	inner_info->name = (char*)p_tbl->name;

	/* --- pthread_t id --- */
	inner_info->pthread_id = pthread_self();

	/* --- init que_worker --- */
	uint8_t nr_que_worker = gp_thm_reg_tbl [inner_info->thread_idx]->nr_que;
	que_worker_t st_que_worker [nr_que_worker]; // このスレッドのque実体
	for (i = 0; i < nr_que_worker; ++ i) {
		clear_que_worker (&st_que_worker [i]);
	}
	inner_info->nr_que_worker = nr_que_worker;
	inner_info->que_worker = &st_que_worker [0];

	/* --- init seq_info --- */
	uint8_t nr_seq = gp_thm_reg_tbl [inner_info->thread_idx]->nr_seq;
	seq_info_t st_seq_info [nr_seq]; // seq_info実体
	for (i = 0; i < nr_seq; ++ i) {
		clear_seq_info (&st_seq_info [i]);
		st_seq_info[i].seq_idx = i;
	}
	inner_info->nr_seq = nr_seq;
	inner_info->seq_info = &st_seq_info [0];


	/* 
	 * create
	 * 登録されていれば行います
	 */
	if (gpfn_dispatcher || p_tbl->pcb_create) {

		if (gpfn_dispatcher) {

			/* c++ wrapper extension */
			gpfn_dispatcher (
				EN_THM_DISPATCH_TYPE_CREATE,
				inner_info->thread_idx,
				SEQ_IDX_BLANK,
				NULL
			);

		} else {

			(void) (p_tbl->pcb_create) ();

		}
	}

	set_state (inner_info->thread_idx, EN_STATE_READY);

	int policy;
	struct sched_param param;
	if (pthread_getschedparam (inner_info->pthread_id, &policy, &param) != 0) {
		THM_PERROR ("pthread_getschedparam()");
	}

	THM_INNER_FORCE_LOG_I (
		"----- %s created. ----- (th_idx:[%d] pthread_id:[%lu] policy:[%s] priority:[%d])\n",
		inner_info->name,
		inner_info->thread_idx,
		inner_info->pthread_id,
		policy == SCHED_FIFO ? "SCHED_FIFO" :
			policy == SCHED_RR ? "SCHED_RR" :
				policy == SCHED_OTHER ? "SCHED_OTHER" :
					"???",
		param.sched_priority
	);

	/* スレッド立ち上がり通知 */
	post_sem ();


	while (1) {

		/*
		 * キューの確認とcondwait
		 */
		check_wait_worker_thread (inner_info);

		/* busy */
		set_state (inner_info->thread_idx, EN_STATE_BUSY);

		/*
		 * キューから取り出す
		 */	
//		rtn_que = deque_worker( inner_info->thread_idx, true );
		rtn_que = check2deque_worker (inner_info->thread_idx, true);
		if (rtn_que.is_used) {

			switch (rtn_que.en_que_type) {
			case EN_QUE_TYPE_SEQ_TIMEOUT:
				/* Seqタイムアウトが無事終わったので タイムアウト情報クリア */
				clear_seq_timeout (inner_info->thread_idx, rtn_que.dest_seq_idx);

			case EN_QUE_TYPE_REQUEST:
			case EN_QUE_TYPE_REPLY:
			case EN_QUE_TYPE_REQ_TIMEOUT:

				/*
				 * request/ reply/ timeout がきました
				 * 登録されているシーケンスを実行します
				 */

				if (gpfn_dispatcher || ((p_tbl->seq_array)+rtn_que.dest_seq_idx)->pcb_seq) {

					/* sect init のrequestキューを保存 */
					//TODO EN_QUE_TYPE_REQUESTで分けるべきか
					if (((inner_info->seq_info)+rtn_que.dest_seq_idx)->en_act == EN_THM_ACT_INIT) {
						memcpy (
							&(((inner_info->seq_info)+rtn_que.dest_seq_idx)->seq_init_que_worker),
							&rtn_que,
							sizeof(que_worker_t)
						);
					}

					/*
					 * 色々と使うから キューまるまる保存
					 * これ以降 st_now_exec_que_workerをクリアするまでget_contextが有効
					 */
					memcpy (&(inner_info->now_exec_que_worker), &rtn_que, sizeof(que_worker_t));

					/* 引数g_thm_src_infoセット */
					p_thm_src_info->thread_idx = rtn_que.src_thread_idx; /* どこのだれから来たのか分かるように */
					p_thm_src_info->seq_idx = rtn_que.src_seq_idx; /* どこのだれから来たのか分かるように */
					p_thm_src_info->req_id = rtn_que.req_id; /* 基本的にはreply時に使う */
					p_thm_src_info->en_rslt = rtn_que.en_rslt; /* reqestの場合EN_THM_RSLT_IGNOREが入る 無効な値
																replyの場合その結果が入る
																Reqタイムアウトの場合はEN_THM_RSLT_REQ_TIMEOUTが入る
																Seqタイムアウトの場合はEN_THM_RSLT_SEQ_TIMEOUTが入る */
					p_thm_src_info->client_id = rtn_que.client_id; /* NOTIFY_CLIENT_ID_BLANK が入る 無効な値 */
					if (rtn_que.msg.is_used) {
						p_thm_src_info->msg.msg = rtn_que.msg.msg;
						p_thm_src_info->msg.size = rtn_que.msg.size;
					}

					/* 引数セット */
					thm_if.src_info = p_thm_src_info;
					thm_if.pfn_reply = reply;
					thm_if.pfn_reg_notify = register_notify;
					thm_if.pfn_unreg_notify = unregister_notify;
					thm_if.pfn_notify = notify;
					thm_if.pfn_set_sectid = set_sect_id;
					thm_if.pfn_get_sectid = get_sect_id;
					thm_if.pfn_set_timeout = set_timeout;
					thm_if.pfn_clear_timeout = clear_timeout;
					thm_if.pfn_enable_overwrite = enable_overwrite;
					thm_if.pfn_disable_overwrite = disable_overwrite;
					thm_if.pfn_lock = lock;
					thm_if.pfn_unlock = unlock;
					thm_if.pfn_get_seq_idx = get_seq_idx;
					thm_if.pfn_get_seq_name = get_seq_name;


					while (1) { // EN_THM_ACT_CONTINUE の為のloopです
						
						// 現在実行中のセクションid 保管します dump用
						get_seq_info (inner_info->thread_idx, rtn_que.dest_seq_idx)->running_sect_id = get_seq_info (inner_info->thread_idx, rtn_que.dest_seq_idx)->sect_id;
						
						/*
						 * 関数実行
						 * 主処理
						 * ユーザ側で sect_id en_actをセットするはず
						 */
						if (gpfn_dispatcher) {


							/* c++ wrapper extension */
							gpfn_dispatcher (
								EN_THM_DISPATCH_TYPE_REQ_REPLY,
								inner_info->thread_idx,
								rtn_que.dest_seq_idx,
								&thm_if
							);


						} else {

							(void) (((p_tbl->seq_array)+rtn_que.dest_seq_idx)->pcb_seq) (&thm_if);

						}

						// 現在実行中のセクションid クリアします dump用
						get_seq_info (inner_info->thread_idx, rtn_que.dest_seq_idx)->running_sect_id = SECT_ID_BLANK;


						if (((inner_info->seq_info)+rtn_que.dest_seq_idx)->en_act == EN_THM_ACT_CONTINUE) {
//TODO
//							/* Seqタイムアウト関係ないとこでは一応クリアする */
//							clear_seq_timeout (inner_info->thread_idx, rtn_que.n_dest_seq_idx);
							continue;

						} else if (((inner_info->seq_info)+rtn_que.dest_seq_idx)->en_act == EN_THM_ACT_WAIT) {

							if (((inner_info->seq_info)+rtn_que.dest_seq_idx)->timeout.val == SEQ_TIMEOUT_BLANK) {
								/* 一応クリア */
								clear_seq_timeout (inner_info->thread_idx, rtn_que.dest_seq_idx);

								/* this while loop break */
								break;

							} else {
								enable_seq_timeout (inner_info->thread_idx, rtn_que.dest_seq_idx);

								/* this while loop break */
								break;
							}

						} else {
							/* Seqタイムアウト関係ないとこでは一応クリアする */
							clear_seq_timeout (inner_info->thread_idx, rtn_que.dest_seq_idx);

							/* this while loop break */
							break;
						}
					}

					/* clear */
					clear_que_worker (&(inner_info->now_exec_que_worker));
					clear_thm_if (&thm_if);
					clear_thm_src_info (p_thm_src_info);
					clear_sync_reply_info (&g_sync_reply_info [inner_info->thread_idx]);
				}

				break;

#if 0 //廃止
			case EN_QUE_TYPE_REPLY:
				/* ここに来るのは非同期のreply */

				if (p_tbl->p_recv_async_reply) {

					//TODO inner_info->st_now_exec_que_worker の保存必要かどうか

					/* 引数g_thm_src_infoセット */
					p_thm_src_info->thread_idx = rtn_que.n_src_thread_idx; /* どこのだれから来たのか分かるように */
					p_thm_src_info->seq_idx = rtn_que.n_src_seq_idx; /* どこのだれから来たのか分かるように */
					p_thm_src_info->req_id = rtn_que.req_id; /* requestの時に生成したもの */
					p_thm_src_info->en_rslt = rtn_que.en_rslt; /* 結果が入る */
					p_thm_src_info->client_id = rtn_que.client_id; /* NOTIFY_CLIENT_ID_BLANK が入る 無効な値 */
					if (rtn_que.msg.is_used) {
						p_thm_src_info->msg = rtn_que.msg.msg;
					}

					/* 引数セット */
					thm_if.src_info = p_thm_src_info;
					thm_if.pfn_reply = NULL;
					thm_if.pfn_reg_notify = NULL;
					thm_if.pfn_un_reg_notify = NULL;
					thm_if.pfn_notify = notify;
					thm_if.pfn_set_sect_id = NULL;
					thm_if.pfn_get_sect_id = NULL;
					thm_if.pfn_set_timeout = NULL;

					/*
					 * 主処理
					 */
					(void)(p_tbl->p_recv_async_reply) (&thm_if);

					/* clear */
					clear_thm_if (&thm_if);
					clear_thm_src_info (p_thm_src_info);
				}

				break;
#endif

			case EN_QUE_TYPE_NOTIFY:
				/* notifyがきました */

				if (gpfn_dispatcher || p_tbl->pcb_recv_notify) {

					/* 引数g_thm_src_infoセット */
					p_thm_src_info->thread_idx = rtn_que.src_thread_idx; /* どこのだれから来たのか分かるように */
					p_thm_src_info->seq_idx = rtn_que.src_seq_idx; /* どこのだれから来たのか分かるように */
					p_thm_src_info->req_id = rtn_que.req_id; /* REQUEST_ID_BLANKが入る 無効な値 */
					p_thm_src_info->en_rslt = rtn_que.en_rslt; /* EN_THM_RSLT_IGNOREが入る 無効な値 */
					p_thm_src_info->client_id = rtn_que.client_id; /* notify時に登録した値 */
					if (rtn_que.msg.is_used) {
						p_thm_src_info->msg.msg = rtn_que.msg.msg;
						p_thm_src_info->msg.size = rtn_que.msg.size;
					}

					/* 引数セット */
					thm_if.src_info = p_thm_src_info;
					thm_if.pfn_reply = NULL;
					thm_if.pfn_reg_notify = NULL;
					thm_if.pfn_unreg_notify = NULL;
					thm_if.pfn_notify = notify;
					thm_if.pfn_set_sectid = NULL;
					thm_if.pfn_get_sectid = NULL;
					thm_if.pfn_set_timeout = NULL;
					thm_if.pfn_clear_timeout = NULL;
					thm_if.pfn_enable_overwrite = NULL;
					thm_if.pfn_disable_overwrite = NULL;
					thm_if.pfn_lock = NULL;
					thm_if.pfn_unlock = NULL;
					thm_if.pfn_get_seq_idx = get_seq_idx;
					thm_if.pfn_get_seq_name = get_seq_name;

					/*
					 * 主処理
					 */
					if (gpfn_dispatcher) {
						/* c++ wrapper extension */
						gpfn_dispatcher (
							EN_THM_DISPATCH_TYPE_NOTIFY,
							inner_info->thread_idx,
							rtn_que.dest_seq_idx,
							&thm_if
						);

					} else {

						(void) (p_tbl->pcb_recv_notify) (&thm_if);
					}

					/* clear */
					clear_thm_if (&thm_if);
					clear_thm_src_info (p_thm_src_info);
				}

				break;

			case EN_QUE_TYPE_DESTROY:
				is_destroy = true;
				break;

			default:
				THM_INNER_LOG_E ("Unexpected: unexpected que_type.\n");
				break;
			}

		} /* rtn_que.is_used */


		/* Reqタイムアウトチェック */
		check_req_timeout	(inner_info->thread_idx);
		/* Seqタイムアウトチェック */
		check_seq_timeout (inner_info->thread_idx);


		if (is_destroy) {
			break;
		}

	} /* main while loop */


	/* 
	 * destroy
	 * 登録されていれば行います
	 */
	if (gpfn_dispatcher || p_tbl->pcb_destroy) {

		if (gpfn_dispatcher) {

			/* c++ wrapper extension */
			gpfn_dispatcher (
				EN_THM_DISPATCH_TYPE_DESTROY,
				inner_info->thread_idx,
				rtn_que.dest_seq_idx,
				NULL
			);

		} else {

			(void) (p_tbl->pcb_destroy) ();

		}
	}

	set_state (inner_info->thread_idx, EN_STATE_DESTROY);
	THM_INNER_FORCE_LOG_I ("----- %s destroyed. -----\n", inner_info->name);

	finaliz_signal_per_thread();

	return NULL;
}

/**
 * clear_thm_if
 */
static void clear_thm_if (threadmgr_if_t *p_if)
{
	if (!p_if) {
		return;
	}

	p_if->src_info = NULL;
	p_if->pfn_reply = NULL;
	p_if->pfn_reg_notify = NULL;
	p_if->pfn_unreg_notify = NULL;
	p_if->pfn_notify = NULL;
	p_if->pfn_set_sectid = NULL;
	p_if->pfn_get_sectid = NULL;
	p_if->pfn_set_timeout = NULL;
	p_if->pfn_clear_timeout = NULL;
	p_if->pfn_enable_overwrite = NULL;
	p_if->pfn_disable_overwrite = NULL;
	p_if->pfn_lock = NULL;
	p_if->pfn_unlock = NULL;
	p_if->pfn_get_seq_idx = NULL;
	p_if->pfn_get_seq_name = NULL;
}

/**
 * clear_thm_src_info
 */
static void clear_thm_src_info (threadmgr_src_info_t *p)
{
	if (!p) {
		return;
	}

	p->thread_idx = THREAD_IDX_BLANK;
	p->seq_idx = SEQ_IDX_BLANK;
	p->req_id = REQUEST_ID_BLANK;
	p->en_rslt = EN_THM_RSLT_IGNORE;
	p->client_id = NOTIFY_CLIENT_ID_BLANK;
	p->msg.msg = NULL;
	p->msg.size= 0;
}

/**
 * request_base_thread
 */
static bool request_base_thread (EN_MONI_TYPE en_type)
{
	/* lock */
	pthread_mutex_lock (&g_mutex_base);

	/* キューに入れる */
	if (!enque_base(en_type)) {
		/* unlock */
		pthread_mutex_unlock (&g_mutex_base);

		THM_INNER_LOG_E ("enque_base() is failure.\n");
		return false;
	}


	/*
	 * Request
	 * cond signal ---> Base_thread
	 */
	pthread_cond_signal (&g_cond_base);


	/* unlock */
	pthread_mutex_unlock (&g_mutex_base);

	return true;
}

/**
 * request_inner
 *
 * 引数 thread_idx, seq_idx は宛先です
 */
static bool request_inner (
	uint8_t thread_idx,
	uint8_t seq_idx,
	uint32_t req_id,
	context_t *context,
	uint8_t *msg,
	size_t msg_size
)
{
	/* lock */
	pthread_mutex_lock (&g_mutex_worker [thread_idx]);

	/* キューに入れる */
	if (!enque_worker (thread_idx, seq_idx, EN_QUE_TYPE_REQUEST,
							context, req_id, EN_THM_RSLT_IGNORE, NOTIFY_CLIENT_ID_BLANK, msg, msg_size)) {
		/* unlock */
		pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

		THM_INNER_LOG_E ("enque_worker() is failure.\n");
		return false;
	}


	/*
	 * Request
	 * cond signal ---> worker_thread
	 */
	pthread_cond_signal (&g_cond_worker [thread_idx]);


	/* unlock */
	pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

	return true;
}

/**
 * Request(同期)
 *
 * 引数 thread_idx, seq_idx は宛先です
 * RequestしたスレッドはReplyが来るまでここで固まる
 * 自分自身に投げたらデッドロックします
 *
 * 公開用 external_if
 */
bool request_sync (uint8_t thread_idx, uint8_t seq_idx, uint8_t *msg, size_t msg_size)
{
	if ((thread_idx < 0) || (thread_idx >= get_total_worker_thread_num())) {
		THM_INNER_LOG_E ("invalid arument\n");
		return false;
	}
	if ((seq_idx < 0) || (seq_idx >= SEQ_IDX_MAX)) {
		/*
		 * 外部のスレッドから呼ぶこともできるから SEQ_IDX_MAX 未満にする
		 * (内部からのみだったら g_inner_info [thread_idx].nr_seq未満でいい)
		 */
		THM_INNER_LOG_E ("invalid arument\n");
		return false;
	}

	uint32_t req_id = REQUEST_ID_BLANK;
	request_id_info_t *p_tmp_req_id_info = NULL;
	sync_reply_info_t *p_tmp_sync_reply_info = NULL;
	external_control_info_t *ext_info = NULL;
	int rtn = 0;
	bool is_need_wait = true;


	context_t ctx = get_context();
	if (!ctx.is_valid) {
		/* --- 外部のスレッドからリクエストの場合の処理 --- */

		ext_info = search_ext_info_list (pthread_self());
		if (!ext_info) {
			/* 事前にcreate_external_cpしてない */
			THM_INNER_LOG_E ("ext_info is not found.  not request... (exec create_external_cp() in advance.)\n");
			return false;
		}

		/* lock */
		pthread_mutex_lock (&(ext_info->mutex));

		if (ext_info->request_option & REQUEST_OPTION__WITHOUT_REPLY) {
			/* 
			 * 同期待ちするのにreply不要になってる...
			 * 無視して続行 ログだけ出しておく
			 */
			THM_INNER_LOG_W ("REQUEST_OPTION__WITHOUT_REPLY is set to wait for synchronization...\n");
		}

		/* request_id */
		req_id = get_request_id (THREAD_IDX_EXTERNAL, SEQ_IDX_BLANK);
		if (req_id == REQUEST_ID_BLANK) {

			/* unlock */
			pthread_mutex_unlock (&(ext_info->mutex));
			return false;
		}

		ext_info->req_id = req_id;

		/* リクエスト投げる */
		if (!request_inner (thread_idx, seq_idx, req_id, &ctx, msg, msg_size)) {
			release_request_id (THREAD_IDX_EXTERNAL, req_id);

			/* unlock */
			pthread_mutex_unlock (&(ext_info->mutex));
			return false;
		}

#ifdef _REQUEST_TIMEOUT
//		enable_req_timeout (THREAD_IDX_EXTERNAL, req_id, REQUEST_TIMEOUT_FIX);
		enable_req_timeout (THREAD_IDX_EXTERNAL, req_id, ext_info->requetimeout_msec);
#endif

		/* unlock */
		pthread_mutex_unlock (&(ext_info->mutex));
		return true;
	}


	if (g_inner_info[ctx.thread_idx].request_option & REQUEST_OPTION__WITHOUT_REPLY) {
		/* 
		 * 同期待ちするのにreply不要になってる...
		 * 無視して続行 ログだけ出しておく
		 */
		THM_INNER_LOG_W ("REQUEST_OPTION__WITHOUT_REPLY is set to wait for synchronization...\n");
	}

	/* request_id */
	req_id = get_request_id (ctx.thread_idx, ctx.seq_idx);
	if (req_id == REQUEST_ID_BLANK) {
		return false;
	}

	if (!set_request_id_sync_reply (ctx.thread_idx, req_id)) {
		/* まだ使われてる.. ありえないけど一応 */
		release_request_id (ctx.thread_idx, req_id);
		return false;
	}

	/* 自分はEN_STATE_WAIT_REPLY にセット */
	set_state (ctx.thread_idx, EN_STATE_WAIT_REPLY);


	/*
	 * !!!ここからg_mutex_sync_reply を lock !!!
	 * こちらがpthread_cond_waitする前に 相手がreply(pthread_cond_signal)してくるのを防ぎます
	 */
//TODO is_request_alreadyで判断
	pthread_mutex_lock (&g_mutex_sync_reply [ctx.thread_idx]);

	//TODO
	// 一度もクリアしてないので ここで一度msg をクリアします
	// そもそもclear_sync_reply_infoをここでやればいいのかも
	// clear_sync_reply_info 内のmsgのクリアのコメントも外せるかも
	p_tmp_sync_reply_info = get_sync_reply_info (ctx.thread_idx);
	if (p_tmp_sync_reply_info) {
		memset (p_tmp_sync_reply_info->msg.msg, 0x00, MSG_SIZE);
		p_tmp_sync_reply_info->msg.size = 0;
		p_tmp_sync_reply_info->msg.is_used = false;
	}

	/* リクエスト投げる */
	if (!request_inner(thread_idx, seq_idx, req_id, &ctx, msg, msg_size)) {

		/* g_mutex_sync_reply unlock */
		pthread_mutex_unlock (&g_mutex_sync_reply[ctx.thread_idx]);

		clear_sync_reply_info (&g_sync_reply_info [ctx.thread_idx]);
		release_request_id (ctx.thread_idx, req_id);
		set_state (ctx.thread_idx, EN_STATE_BUSY);

		return false;
	}

#ifndef _REQUEST_TIMEOUT
	THM_INNER_LOG_D ("request_sync... cond wait\n");

	/* 自分はcond wait して固まる(Reply待ち) */
	while (is_need_wait) {
		n_rtn = pthread_cond_wait (
			&g_cond_sync_reply [ctx.thread_idx],
			&g_mutex_sync_reply [ctx.thread_idx]
		);
		switch (n_rtn) {
		case EINTR:
			THM_INNER_LOG_W ("interrupt occured.\n");
			break;
		default:
			is_need_wait = false;
			break;
		}
	}
#else
//	enable_req_timeout (ctx.thread_idx, req_id, REQUEST_TIMEOUT_FIX);
	enable_req_timeout (ctx.thread_idx, req_id, g_inner_info[ctx.thread_idx].requetimeout_msec);

	p_tmp_req_id_info = get_request_id_info (ctx.thread_idx, req_id);
	if (!p_tmp_req_id_info) {
		/* NULLリターンは起こりえないはず */
		THM_INNER_LOG_E ("get_request_id_info is null\n");

		/* g_mutex_sync_reply unlock */
		pthread_mutex_unlock (&g_mutex_sync_reply[ctx.thread_idx]);

		clear_sync_reply_info (&g_sync_reply_info [ctx.thread_idx]);
		release_request_id (ctx.thread_idx, req_id);
		set_state (ctx.thread_idx, EN_STATE_BUSY);

		return false;
	}

	if (p_tmp_req_id_info->timeout.en_state == EN_TIMEOUT_STATE_INIT) {
		THM_INNER_LOG_D ("request_sync... cond wait\n");

		/* 自分はcond wait して固まる(Reply待ち) */
		while (is_need_wait) {
			rtn = pthread_cond_wait (
				&g_cond_sync_reply [ctx.thread_idx],
				&g_mutex_sync_reply [ctx.thread_idx]
			);
			switch (rtn) {
			case EINTR:
				THM_INNER_LOG_W ("interrupt occured.\n");
				break;
			default:
				is_need_wait = false;
				break;
			}
		}
		
	} else {
		THM_INNER_LOG_D ("request_sync... cond timedwait\n");

		/* 自分はcond wait して固まる(Reply待ち) */
		while (is_need_wait) {
			rtn = pthread_cond_timedwait (
				&g_cond_sync_reply [ctx.thread_idx],
				&g_mutex_sync_reply [ctx.thread_idx],
				&(p_tmp_req_id_info->timeout.time)
			);
			switch (rtn) {
			case EINTR:
				THM_INNER_LOG_W ("interrupt occured.\n");
				break;
			default:
				is_need_wait = false;
				break;
			}
		}
	}
#endif

	p_tmp_sync_reply_info = get_sync_reply_info (ctx.thread_idx);

	switch (rtn) {
	case SYS_RETURN_NORMAL:
		THM_INNER_LOG_D ("request_sync... reply come\n");

		/*
		 * リプライが来た
		 * 結果をいれる
		 */
		if (p_tmp_sync_reply_info) {
			g_thm_src_info [ctx.thread_idx].thread_idx = thread_idx;
			g_thm_src_info [ctx.thread_idx].seq_idx = seq_idx;
			g_thm_src_info [ctx.thread_idx].req_id = req_id;
			g_thm_src_info [ctx.thread_idx].en_rslt = p_tmp_sync_reply_info->en_rslt;
			if (p_tmp_sync_reply_info->msg.is_used) {
				g_thm_src_info [ctx.thread_idx].msg.msg = p_tmp_sync_reply_info->msg.msg;
				g_thm_src_info [ctx.thread_idx].msg.size = p_tmp_sync_reply_info->msg.size;
			} else {
				g_thm_src_info [ctx.thread_idx].msg.msg = NULL;
				g_thm_src_info [ctx.thread_idx].msg.size = 0;
			}
		}
		break;

	case ETIMEDOUT:
		THM_INNER_LOG_W ("request_sync... timeout\n");

		/*
		 * タイムアウトした
		 */
		if (p_tmp_sync_reply_info) {
			clear_thm_src_info (&g_thm_src_info [ctx.thread_idx]);
			// タイムアウトしたことだけわかるようにしておきます
			g_thm_src_info [ctx.thread_idx].en_rslt = EN_THM_RSLT_REQ_TIMEOUT;
		}
		break;

//	case EINTR:
//		break;

	default:
		THM_INNER_LOG_E ("Unexpected: pthread_cond_timedwait() => unexpected return value [%d]\n", rtn);
		break;
	}

	/* g_mutex_sync_reply unlock */
	pthread_mutex_unlock (&g_mutex_sync_reply[ctx.thread_idx]);


	/*
	 * main loop側でセクション終わりにクリアしているけど先にここでもクリアします
	 * セクション内で複数回request_syncした場合などに対応
//TODO なぜなのか思い出したい
	 */
	clear_sync_reply_info (&g_sync_reply_info [ctx.thread_idx]);

	release_request_id (ctx.thread_idx, req_id);
	set_state (ctx.thread_idx, EN_STATE_BUSY);


	return true;
}

/**
 * Request(非同期)
 *
 * 引数 thread_idx, seq_idx は宛先です
 * p_req_idはout引数です
 *
 * 公開用 external_if
 */
bool request_async (uint8_t thread_idx, uint8_t seq_idx, uint8_t *msg, size_t msg_size, uint32_t *out_req_id)
{
	if ((thread_idx < 0) || (thread_idx >= get_total_worker_thread_num())) {
		THM_INNER_LOG_E ("invalid arument\n");
		return false;
	}
	if ((seq_idx < 0) || (seq_idx >= SEQ_IDX_MAX)) {
//TODO ?? SEQ_IDX_MAX 未満になってない 以下になってる??
		/*
		 * 外部のスレッドから呼ぶこともできるから SEQ_IDX_MAX 未満にする
		 * (内部からのみだったら g_inner_info [thread_idx].nr_seq未満でいい)
		 */
		THM_INNER_LOG_E ("invalid arument\n");
		return false;
	}

	uint32_t req_id = REQUEST_ID_BLANK;
	external_control_info_t *ext_info = NULL;


	context_t ctx = get_context();
	if (!ctx.is_valid) {
		/* --- 外部のスレッドからリクエストの場合の処理 --- */

		ext_info = search_ext_info_list (pthread_self());
		if (!ext_info) {
			/* 事前にcreate_external_cpしてない */
			THM_INNER_LOG_E ("ext_info is not found.  not request... (exec create_external_cp() in advance.)\n");
			return false;
		}

		/* lock */
		pthread_mutex_lock (&(ext_info->mutex));

		if (ext_info->request_option & REQUEST_OPTION__WITHOUT_REPLY) {
			/* reply不要 req_idは取得しない */
			req_id = REQUEST_ID_UNNECESSARY;

		} else {
			/* request_id */
			req_id = get_request_id (THREAD_IDX_EXTERNAL, SEQ_IDX_BLANK);
			if (req_id == REQUEST_ID_BLANK) {

				/* unlock */
				pthread_mutex_unlock (&(ext_info->mutex));
				return false;
			}
		}

		/* 引数out_req_idに返却 */
		if (out_req_id) {
			*out_req_id = req_id;
		}

		/*
		 * req_idをext_infoにセット
		 * 上書きなので 外部スレッドからは1つづつのrequestしかできません
		 */
		ext_info->req_id = req_id;

		/* リクエスト投げる */
		if (!request_inner (thread_idx, seq_idx, req_id, &ctx, msg, msg_size)) {
			release_request_id (THREAD_IDX_EXTERNAL, req_id);

			/* unlock */
			pthread_mutex_unlock (&(ext_info->mutex));
			return false;
		}

#ifdef _REQUEST_TIMEOUT
//		enable_req_timeout (THREAD_IDX_EXTERNAL, req_id, REQUEST_TIMEOUT_FIX);
		enable_req_timeout (THREAD_IDX_EXTERNAL, req_id, ext_info->requetimeout_msec);
#endif

		/* unlock */
		pthread_mutex_unlock (&(ext_info->mutex));
		return true;
	}


	if (g_inner_info[ctx.thread_idx].request_option & REQUEST_OPTION__WITHOUT_REPLY) {
		/* reply不要 req_idは取得しない */
		req_id = REQUEST_ID_UNNECESSARY;

	} else {
		/* request_id */
		req_id = get_request_id (ctx.thread_idx, ctx.seq_idx);
		if (req_id == REQUEST_ID_BLANK) {
			return false;
		}
	}

	/* 引数p_req_idに返却 */
	if (out_req_id) {
		*out_req_id = req_id;
	}

#ifndef _MULTI_REQUESTING
	/*
	 * req_idを自身のseq_infoに保存
	 * replyが返ってきたとき check2deque_worker()で照合するため
	 */
	if (ctx.is_valid) {
		uint8_t n_context_seq_idx = ctx.seq_idx;
		get_seq_info (ctx.thread_idx, n_context_seq_idx)->req_id = req_id;
	}
#endif

	/* リクエスト投げる */
	if (!request_inner(thread_idx, seq_idx, req_id, &ctx, msg, msg_size)) {
		release_request_id (ctx.thread_idx, req_id);
		return false;
	}

#ifdef _REQUEST_TIMEOUT
//	enable_req_timeout (ctx.thread_idx, req_id, REQUEST_TIMEOUT_FIX);
	enable_req_timeout (ctx.thread_idx, req_id, g_inner_info[ctx.thread_idx].requetimeout_msec);
#endif

	return true;
}

/**
 * set_request_option
 *
 * 公開用 external_if
 */
void set_request_option (uint32_t option)
{
	if ((option & REQUEST_OPTION__WITH_TIMEOUT_MSEC) && (option & REQUEST_OPTION__WITH_TIMEOUT_MIN)) {
		THM_INNER_FORCE_LOG_W ("REQUEST_OPTION__WITH_TIMEOUT_MSEC / MIN both set...\n");
		THM_INNER_FORCE_LOG_W ("force set REQUEST_OPTION__WITH_TIMEOUT_MIN\n");
		option &= ~REQUEST_OPTION__WITH_TIMEOUT_MSEC;
	}

	context_t ctx = get_context();
	if (!ctx.is_valid) {
		/* --- 外部のスレッドの場合 --- */

		external_control_info_t *ext_info = search_ext_info_list (pthread_self());
		if (!ext_info) {
			/* 事前にcreate_external_cpしてない */
			THM_INNER_LOG_E ("ext_info is not found.  not request... (exec create_external_cp() in advance.)\n");
			return ;
		}

		ext_info->request_option = option;
		ext_info->requetimeout_msec = apply_timeout_msec_by_request_option (option);

	} else {
		g_inner_info [ctx.thread_idx].request_option = option;
		g_inner_info [ctx.thread_idx].requetimeout_msec = apply_timeout_msec_by_request_option (option);
	}
}

/**
 * get_request_option
 *
 * 公開用 external_if
 */
uint32_t get_request_option (void)
{
	context_t ctx = get_context();
	if (!ctx.is_valid) {
		/* --- 外部のスレッドの場合 --- */

		external_control_info_t *ext_info = search_ext_info_list (pthread_self());
		if (!ext_info) {
			/* 事前にcreate_external_cpしてない */
			THM_INNER_LOG_E ("ext_info is not found.  not request... (exec create_external_cp() in advance.)\n");
			return 0;
		}

		return ext_info->request_option;

	} else {
		return g_inner_info [ctx.thread_idx].request_option;
	}
}

/**
 * apply_timeout_msec_by_request_option
 */
uint32_t apply_timeout_msec_by_request_option (uint32_t option)
{
	uint32_t _t_o = (option >> 16) & 0xffff;

	if (option & REQUEST_OPTION__WITH_TIMEOUT_MSEC) {
		return _t_o;

	} else if (option & REQUEST_OPTION__WITH_TIMEOUT_MIN) {
		return _t_o * 60 * 1000;

	} else {
		return 0;
	}
}

/**
 * reply_inner
 *
 * 引数 thread_idx, seq_idx は宛先です req_idはRequest元の値
 */
static bool reply_inner (
	uint8_t thread_idx,
	uint8_t seq_idx,
	uint32_t req_id,
	context_t *context,
	EN_THM_RSLT en_rslt,
	uint8_t *msg,
	size_t msg_size,
	bool is_sync
)
{
	if (is_sync) {
		/* sync Reply */
		if (!cache_sync_reply_info (thread_idx, en_rslt, msg, msg_size)) {
			THM_INNER_LOG_E ("cache_sync_reply_info() is failure.\n");
			return false;
		}

		pthread_mutex_lock (&g_mutex_sync_reply [thread_idx]);

		pthread_cond_signal (&g_cond_sync_reply [thread_idx]);

		set_reply_already_sync_reply_info (thread_idx);
		pthread_mutex_unlock (&g_mutex_sync_reply [thread_idx]);

		return true;
	}


	/* lock */
	pthread_mutex_lock (&g_mutex_worker [thread_idx]);


#if 0
#ifdef _REQUEST_TIMEOUT
	/* Reqタイムアウトしてないか確認する */
	EN_TIMEOUT_STATE en_state = get_req_timeout_state (thread_idx, req_id);
	if ((en_state != EN_TIMEOUT_STATE_MEAS) && (en_state != EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT)) {
		THM_INNER_FORCE_LOG_W ("get_req_timeout_state() is unexpected. [%d]   maybe timeout occured. not reply...\n", en_state);

		/* unlock */
		pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

		return false;
	}
#endif
#endif

	/* キューに入れる */
	if (!enque_worker (thread_idx, seq_idx, EN_QUE_TYPE_REPLY,
							context, req_id, en_rslt, NOTIFY_CLIENT_ID_BLANK, msg, msg_size)) {
		/* unlock */
		pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

		THM_INNER_LOG_E ("enque_worker() is failure.\n");
		return false;
	}


	/* 
	 * Reply
	 * cond signal ---> worker_thread
	 */
	pthread_cond_signal (&g_cond_worker [thread_idx]);


	/* unlock */
	pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

	return true;
}

/**
 * reply_outer
 * 外部スレッドへのリプライ
 */
static bool reply_outer (
	uint32_t req_id,
	context_t *context,
	EN_THM_RSLT en_rslt,
	uint8_t *msg,
	size_t msg_size
)
{
	external_control_info_t *ext_info = NULL;

	ext_info = search_ext_info_list_from_request_id (req_id);
	if (!ext_info) {
		/* create_external_cp してない or  別のrequestで書き換わった or すでにタイムアウト */
		THM_INNER_LOG_W ("ext_info is not found.  not reply...\n");
		return false;
	}

	/* lock */
	pthread_mutex_lock (&(ext_info->mutex));

	/* 結果を入れます */
	ext_info->thm_src_info.thread_idx = context->thread_idx;
	ext_info->thm_src_info.seq_idx = context->seq_idx;
	if (en_rslt == EN_THM_RSLT_REQ_TIMEOUT) {
		/* タイムアウトだったら ext_infoのreq_idクリア */
		ext_info->req_id = REQUEST_ID_BLANK;
		ext_info->thm_src_info.req_id = REQUEST_ID_BLANK;
	} else {
		ext_info->thm_src_info.req_id = req_id;
	}
	ext_info->thm_src_info.en_rslt = en_rslt;
	if (msg && msg_size > 0) {
		memset (ext_info->msg_entity.msg, 0x00, MSG_SIZE);
		memcpy (ext_info->msg_entity.msg, msg, msg_size < MSG_SIZE ? msg_size : MSG_SIZE);
		ext_info->msg_entity.size = msg_size < MSG_SIZE ? msg_size : MSG_SIZE;
		ext_info->thm_src_info.msg.msg = ext_info->msg_entity.msg;
		ext_info->thm_src_info.msg.size = ext_info->msg_entity.size;
	} else {
		ext_info->thm_src_info.msg.msg = NULL;
		ext_info->thm_src_info.msg.size = 0;
	}

	pthread_cond_signal (&(ext_info->cond));
	ext_info->is_reply_already = true;

	/* unlock */
	pthread_mutex_unlock (&(ext_info->mutex));

	return true;
}

/**
 * Reply
 * 公開用
 */
static bool reply (EN_THM_RSLT en_rslt, uint8_t *msg, size_t msg_size)
{
	/*
	 * get_context->自分のthread_idx取得->inner_infoを参照して返送先を得る
	 */

	context_t ctx = get_context();

	if (!ctx.is_valid) {
		/* ありえないけど一応 */
		return false;
	}

	uint8_t thread_idx = THREAD_IDX_BLANK;
	uint8_t seq_idx = SEQ_IDX_BLANK;
	uint32_t req_id = REQUEST_ID_BLANK;
	bool is_sync = false;

	bool is_valid = get_seq_info (ctx.thread_idx, ctx.seq_idx)->seq_init_que_worker.is_valid_src_info;
	if (!is_valid) {
		/* 返送先がわからない --> ということは外部のスレッドからのリクエストです */

		req_id = get_seq_info (ctx.thread_idx, ctx.seq_idx)->seq_init_que_worker.req_id;
		if (req_id == REQUEST_ID_UNNECESSARY) {
			/* replyする必要なければ とくに何もしない */
			//THM_INNER_FORCE_LOG_D ("REQUEST_ID_UNNECESSARY\n");
			return true;
		}

		if (!is_active_request_id (THREAD_IDX_EXTERNAL, req_id)) {
			THM_INNER_FORCE_LOG_W ("req_id:[0x%x] is in_active. maybe timeout occured. not reply...\n", req_id);
			return false;
		}

		/* リプライ投げる */
		if (!reply_outer (req_id, &ctx, en_rslt, msg, msg_size)) {
			return false;
		}

	} else {
		/* 内部へ通常リプライ */

		thread_idx = get_seq_info (ctx.thread_idx, ctx.seq_idx)->seq_init_que_worker.src_thread_idx;
		seq_idx = get_seq_info (ctx.thread_idx, ctx.seq_idx)->seq_init_que_worker.src_seq_idx;
		req_id = get_seq_info (ctx.thread_idx, ctx.seq_idx)->seq_init_que_worker.req_id;
		if (req_id == REQUEST_ID_UNNECESSARY) {
			/* replyする必要なければ とくに何もしない */
			//THM_INNER_FORCE_LOG_D ("REQUEST_ID_UNNECESSARY\n");
			return true;
		}

		if (!is_active_request_id (thread_idx, req_id)) {
			THM_INNER_FORCE_LOG_W ("req_id:[0x%x] is in_active. maybe timeout occured. not reply...\n", req_id);
			return false;
		}

		is_sync = is_sync_reply_from_request_id (thread_idx, req_id);

		/* リプライ投げる */
		if (!reply_inner (thread_idx, seq_idx, req_id, &ctx, en_rslt, msg, msg_size, is_sync)) {
			return false;
		}
	}

	return true;
}

/**
 * get_context
 * これを実行したコンテキストがどのスレッドなのか把握する
 */
static context_t get_context (void)
{
	int i = 0;
	context_t ctx;

	/* pthread_self() */
	pthread_t n_current_pthread_id = pthread_self();

	/* init clear */
	clear_context (&ctx);

	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		if (pthread_equal (g_inner_info [i].pthread_id, n_current_pthread_id)) {
			break;
		}
	}

	if (i < get_total_worker_thread_num()) {
		ctx.is_valid = true;
		ctx.thread_idx = g_inner_info [i].thread_idx;
		ctx.seq_idx = g_inner_info [i].now_exec_que_worker.dest_seq_idx;

	} else {
		/* not found */
		/* フレームワーク管理外のスレッドです */
		THM_INNER_LOG_W ("this framework unmanaged thread. pthread_id:[%lu]\n", n_current_pthread_id);
	}

	return ctx;
}

/**
 * clear_context
 */
static void clear_context (context_t *p)
{
	if (!p) {
		return;
	}

	p->is_valid = false;
	p->thread_idx = THREAD_IDX_BLANK;
	p->seq_idx = SEQ_IDX_BLANK;
}

/**
 * get_request_id
 * (REQUEST_ID_MAX -1 で一周する)
 * Reply時にどのRequestのものか判別したり、Reqタイムアウト判定するためのもの
 * 
 * 引数 thread_idx seq_idx は リクエスト元の値です
 * REQUEST_ID_BLANKが戻ったらエラーです
 */
static uint32_t get_request_id (uint8_t thread_idx, uint8_t seq_idx)
{
	if ((thread_idx < 0) || (thread_idx >= get_total_worker_thread_num())) {
//TODO 引数チェックおかしい?
		/* 外部スレッドを考慮 */
		THM_INNER_LOG_D ("external request\n");
		thread_idx = THREAD_IDX_EXTERNAL;
	} else {
		// 内部から呼ばれた
		if ((seq_idx < 0) || (seq_idx >= g_inner_info [thread_idx].nr_seq)) {
			THM_INNER_LOG_E ("invalid arument\n");
			return REQUEST_ID_BLANK;
		}
	}

	uint32_t n_rtreq_id = REQUEST_ID_BLANK;
	uint32_t n = 0;


	/* lock */
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);


	while (n < REQUEST_ID_MAX) {
		if (g_request_id_info [thread_idx][g_nr_request_id_ind[thread_idx]].id != REQUEST_ID_BLANK) {
			/* next */
			g_nr_request_id_ind [thread_idx] ++;
			g_nr_request_id_ind [thread_idx] &= REQUEST_ID_MAX -1;
		} else {
			break;
		}
		++ n;
	}

	if (n == REQUEST_ID_MAX) {
		/* 空きがない エラー */
		THM_INNER_LOG_E ("request id is full.\n");

		/* unlock  */
		pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);

		return REQUEST_ID_BLANK;
	}

	n_rtreq_id = g_nr_request_id_ind [thread_idx];
	g_request_id_info [thread_idx][n_rtreq_id].id = n_rtreq_id;
	g_request_id_info [thread_idx][n_rtreq_id].src_thread_idx = thread_idx;
	g_request_id_info [thread_idx][n_rtreq_id].src_seq_idx = seq_idx;


	/* for next set */
	g_nr_request_id_ind [thread_idx] ++;
	g_nr_request_id_ind [thread_idx] &= REQUEST_ID_MAX -1;


	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);

	return n_rtreq_id;
}

/**
 * dump_request_id
 * デバッグ用
 */
static void dump_request_id_info (void)
{
	int i = 0;
	int j = 0;
	bool is_found = false;

//TODO 参照だけ ログだけだからmutexしない

	THM_LOG_I ("####  dump request_id_info  ####\n");

	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		THM_LOG_I (" --- thread:[%s]\n", g_inner_info [i].name);
		for (j = 0; j < REQUEST_ID_MAX; ++ j) {
			if (g_request_id_info [i][j].id != REQUEST_ID_BLANK) {
				THM_LOG_I (
					"  0x%02x - 0x%02x 0x%02x %s\n",
					g_request_id_info [i][j].id,
					g_request_id_info [i][j].src_thread_idx,
					g_request_id_info [i][j].src_seq_idx,
					gpsz_timeout_state [g_request_id_info [i][j].timeout.en_state]
				);
				is_found = true;
			}
		}
		if (!is_found) {
			THM_LOG_I ("  none\n");
		}
	}

	/* 外部スレッド */
	THM_LOG_I (" --- thread:external\n");
	for (j = 0; j < REQUEST_ID_MAX; ++ j) {
		if (g_request_id_info [THREAD_IDX_EXTERNAL][j].id != REQUEST_ID_BLANK) {
			THM_LOG_I (
				"  0x%02x - 0x%02x 0x%02x %s\n",
				g_request_id_info [THREAD_IDX_EXTERNAL][j].id,
				g_request_id_info [THREAD_IDX_EXTERNAL][j].src_thread_idx,
				g_request_id_info [THREAD_IDX_EXTERNAL][j].src_seq_idx,
				gpsz_timeout_state [g_request_id_info [THREAD_IDX_EXTERNAL][j].timeout.en_state]
			);
			is_found = true;
		}
	}
	if (!is_found) {
		THM_LOG_I ("  none\n");
	}
}

/**
 * get_request_id_info
 */
static request_id_info_t *get_request_id_info (uint8_t thread_idx, uint32_t req_id)
{
	request_id_info_t *p_info = NULL;

	/* lock */
//TODO 参照のみなのでmutexいらないかも
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);

	p_info = &g_request_id_info [thread_idx][req_id];

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);

	return p_info;
}

/**
 * is_active_request_id
 * req_idが生きているか reply時のチェック
 * ここは参照のみだが他スレッドからも呼ばれるところなのでmutex
 */
static bool is_active_request_id (uint8_t thread_idx, uint32_t req_id)
{
	bool rtn = false;

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);

	if (g_request_id_info [thread_idx][req_id].id != REQUEST_ID_BLANK) {
		rtn = true;
	}

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);

	return rtn;
}

/**
 * enable_req_timeout
 * Reqタイムアウトを仕掛ける
 */
static void enable_req_timeout (uint8_t thread_idx, uint32_t req_id, uint32_t timeout_msec)
{
	if ((thread_idx < 0) || (thread_idx >= get_total_worker_thread_num())) {
//TODO 引数チェックおかしい?
		/* 外部スレッドを考慮 */
		THM_INNER_LOG_D ("external\n");
		thread_idx = THREAD_IDX_EXTERNAL;
	}

	if (req_id == REQUEST_ID_UNNECESSARY) {
		return;
	}
	if ((req_id < 0) || (req_id >= REQUEST_ID_MAX)) {
		THM_INNER_LOG_E ("invalid arg req_id.\n");
		return;
	}

	if (timeout_msec < 0) {
		timeout_msec = 0;
	} else if (timeout_msec > REQUEST_TIMEOUT_MAX) {
		timeout_msec = REQUEST_TIMEOUT_MAX;
	}
	if (timeout_msec == 0) {
		return;
	}

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);

	if (g_request_id_info [thread_idx][req_id].timeout.en_state != EN_TIMEOUT_STATE_INIT) {
		THM_INNER_LOG_E ("Unexpected: timeout.en_state != EN_TIMEOUT_STATE_INIT\n");

		/* unlock */
		pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);

		return;
	}

	struct timeval now_timeval = { 0 };
	get_time_of_day (&now_timeval);

	long usec = now_timeval.tv_usec + (timeout_msec % 1000) * 1000;
	if ((usec / 1000000) > 0) {
		g_request_id_info [thread_idx][req_id].timeout.time.tv_sec = now_timeval.tv_sec + (timeout_msec / 1000) + (usec / 1000000);
		g_request_id_info [thread_idx][req_id].timeout.time.tv_nsec = (usec % 1000000) * 1000; // nsec

	} else {
		g_request_id_info [thread_idx][req_id].timeout.time.tv_sec = now_timeval.tv_sec + (timeout_msec / 1000);
		g_request_id_info [thread_idx][req_id].timeout.time.tv_nsec = usec * 1000; // nsec
	}
	g_request_id_info [thread_idx][req_id].timeout.en_state = EN_TIMEOUT_STATE_MEAS;

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);
}

/**
 * check_req_timeout
 * タイムアウトしたかどうかの確認して キュー入れる
 *
 * 一連の登録シーケンス実行後にチェック
 * 外部スレッドは base_threadでチェック
 */
static void check_req_timeout (uint8_t thread_idx)
{
	context_t ctx;
	clear_context (&ctx);

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);

	uint32_t i = 0; //req_id
	for (i = 0; i < REQUEST_ID_MAX; ++ i) {
		if (g_request_id_info [thread_idx][i].timeout.en_state == EN_TIMEOUT_STATE_MEAS) {
			if (is_req_timeout_from_request_id (thread_idx, i)) {
				if (thread_idx == THREAD_IDX_EXTERNAL) {
					/*
					 * 外部スレッドのReqタイムアウト
					 * ここでreplyして req_id解放します
					 */
					THM_INNER_FORCE_LOG_I ("external thread -- req_timeout req_id:[0x%x]\n", i);
					reply_outer (i, &ctx, EN_THM_RSLT_REQ_TIMEOUT, NULL, 0);
					release_request_id (THREAD_IDX_EXTERNAL, i);

				} else {
					/* 自スレッドにReqタイムアウトのキューを入れる */
					g_request_id_info [thread_idx][i].timeout.en_state = EN_TIMEOUT_STATE_PASSED;
					enque_req_timeout (thread_idx, i);
				}
			}
		}
	}

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);
}

/**
 * is_req_timeout_from_request_id
 */
static bool is_req_timeout_from_request_id (uint8_t thread_idx, uint32_t req_id)
{
	// check_req_timeout から呼ばれるので ここはmutex済み

	bool is_timeout = false;
	struct timeval now_timeval = {0};
	get_time_of_day (&now_timeval);
	time_t now_sec = now_timeval.tv_sec;
	long now_nsec  = now_timeval.tv_usec * 1000;

	time_t timeout_sec = g_request_id_info [thread_idx][req_id].timeout.time.tv_sec;
	long timeout_nsec  = g_request_id_info [thread_idx][req_id].timeout.time.tv_nsec;

	time_t diff_sec = 0;
	long diff_nsec = timeout_nsec - now_nsec;
	if (diff_nsec >= 0) {
		diff_sec = timeout_sec - now_sec;
	} else {
		diff_sec = timeout_sec -1 - now_sec;
		diff_nsec += 1000000000;
	}

	if (diff_sec > 0) {
		/* まだタイムアウトしてない */
		THM_INNER_LOG_I ("not yet timeout(Req_timeout) thread_idx:[%d] req_id:[0x%x]\n", thread_idx, req_id);

	} else if (diff_sec == 0) {
		if (diff_nsec > 0) {
			/* まだタイムアウトしてない 1秒切ってる */
			THM_INNER_LOG_I ("not yet timeout - off the one second (seq_timeout) thread_idx:[%d] req_id:[0x%x]\n", thread_idx, req_id);

		} else if (diff_nsec == 0) {
			/* ちょうどタイムアウト かなりレア... */
			THM_INNER_LOG_I ("just timedout(Req_timeout) thread_idx:[%d] req_id:[0x%x]\n", thread_idx, req_id);
			is_timeout = true;

		} else {
			/* ありえない */
			THM_INNER_LOG_E ("Unexpected: diff_nsec is minus value.\n");
		}

	} else {
		/* 既にタイムアウトしてる... */
		THM_INNER_LOG_I ("already timedout(Req_timeout) thread_idx:[%d] req_id:[0x%x]\n", thread_idx, req_id);
		is_timeout = true;
	}

	return is_timeout;
}

/**
 * enque_req_timeout
 */
static bool enque_req_timeout (uint8_t thread_idx, uint32_t req_id)
{
	context_t ctx = get_context();

	/* lock */
	/* 再帰mutexする場合あります */
//TODO 同スレッドからのみ呼ばれる 参照のみなのでmutexいらないかも
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);


	uint8_t seq_idx = g_request_id_info [thread_idx][req_id].src_seq_idx;

	if (!enque_worker (thread_idx, seq_idx, EN_QUE_TYPE_REQ_TIMEOUT,
							&ctx, req_id, EN_THM_RSLT_REQ_TIMEOUT, NOTIFY_CLIENT_ID_BLANK, NULL, 0)) {
		THM_INNER_LOG_E ("enque_worker() is failure.\n");

		/* unlock */
		pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);

		return false;
	}

	THM_INNER_LOG_I ("enque REQ_TIMEOUT %s req_id:[0x%x]\n", g_inner_info [thread_idx].name, req_id);

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);

	return true;
}

/**
 * get_req_timeout_state
 * ここは参照のみだが他スレッドからも呼ばれるところなのでmutex reply時のチェック
 */
static EN_TIMEOUT_STATE get_req_timeout_state (uint8_t thread_idx, uint32_t req_id)
{
	EN_TIMEOUT_STATE en_state = EN_TIMEOUT_STATE_INIT;

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);

	en_state = g_request_id_info [thread_idx][req_id].timeout.en_state;

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);

	return en_state;
}

/**
 * search_nearest_req_timeout
 * Reqタイムアウト仕掛り中のもので 最も近い時間のものを探す
 * request_id_info_t で返します
 * タイムアウト仕掛けてなければnullを返します
 *
 * cond_waitする直前にチェックします
 * pthread_cond_wait か pthread_cond_timedwaitにするかの判断
 */
static request_id_info_t *search_nearest_req_timeout (uint8_t thread_idx)
{
	time_t timeout_sec_min = 0;
	time_t timeout_nsec_min = 0;
	time_t timeout_sec = 0;
	long timeout_nsec  = 0;
	uint8_t n_rtreq_id = REQUEST_ID_MAX;

	/* lock */
//TODO 同スレッドからのみ呼ばれる 参照のみなのでmutexいらないかも
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);

	int i = 0;
	for (i = 0; i < REQUEST_ID_MAX; ++ i) {
		if (g_request_id_info [thread_idx][i].timeout.en_state == EN_TIMEOUT_STATE_MEAS) {

			if ((timeout_sec_min == 0) && (timeout_nsec_min == 0)) {
				// for 1順目
				timeout_sec_min = g_request_id_info [thread_idx][i].timeout.time.tv_sec;
				timeout_nsec_min = g_request_id_info [thread_idx][i].timeout.time.tv_nsec;
				n_rtreq_id = i;

			} else {
				timeout_sec = g_request_id_info [thread_idx][i].timeout.time.tv_sec;
				timeout_nsec = g_request_id_info [thread_idx][i].timeout.time.tv_nsec;

				if (timeout_sec < timeout_sec_min) {
					timeout_sec_min = timeout_sec;
					timeout_nsec_min = timeout_nsec;
					n_rtreq_id = i;

				} else if (timeout_sec == timeout_sec_min) {
					if (timeout_nsec < timeout_nsec_min) {
						timeout_sec_min = timeout_sec;
						timeout_nsec_min = timeout_nsec;
						n_rtreq_id = i;
					}
				}
			}
		}
	}

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);


	if (n_rtreq_id == REQUEST_ID_MAX) {
		/* タイムアウト仕掛かけてない */
		return NULL;
	} else {
		return &g_request_id_info [thread_idx][n_rtreq_id];
	}
}

/**
 * release_request_id
 * req_idを解放します
 *
 * reply受けた時やReqタイムアウト等の回収したタイミングで実行します
 */
static void release_request_id (uint8_t thread_idx, uint32_t req_id)
{
	if ((thread_idx < 0) || (thread_idx >= get_total_worker_thread_num())) {
//TODO 引数チェックおかしい?
		/* 外部スレッドを考慮 */
		THM_INNER_LOG_D ("external\n");
		thread_idx = THREAD_IDX_EXTERNAL;
	}
	if ((req_id < 0) || (req_id >= REQUEST_ID_MAX)) {
		if (req_id == REQUEST_ID_BLANK) {
			THM_INNER_FORCE_LOG_I ("arg req_id is REQUEST_ID_BLANK.\n");
		} else {
			THM_INNER_LOG_E ("invalid arg req_id. req_id:[0x%x]\n", req_id);
		}
		return;
	}

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_request_id [thread_idx]);

	if (g_request_id_info [thread_idx][req_id].id == REQUEST_ID_BLANK) {
		THM_INNER_FORCE_LOG_I ("req_id:[0x%x] is already released ???\n", req_id);
	} else {
		clear_request_id_info (&g_request_id_info [thread_idx][req_id]);
//		THM_INNER_FORCE_LOG_W ("req_id:[0x%x] is released.\n", req_id);
	}

	/* unlock  */
	pthread_mutex_unlock (&g_mutex_ope_request_id [thread_idx]);
}

/**
 * clear_request_id_info
 */
static void clear_request_id_info (request_id_info_t *p)
{
	if (!p) {
		return;
	}

	p->id = REQUEST_ID_BLANK;
	p->src_thread_idx = THREAD_IDX_BLANK;
	p->src_seq_idx = SEQ_IDX_BLANK;
	p->timeout.en_state = EN_TIMEOUT_STATE_INIT;
	memset (&(p->timeout.time), 0x00, sizeof(struct timespec));
}

/**
 * is_sync_reply_from_request_id
 * 同期reply待ち用
 */
static bool is_sync_reply_from_request_id (uint8_t thread_idx, uint32_t req_id)
{
	if (g_sync_reply_info [thread_idx].req_id == req_id) {
		return true;
	} else {
		return false;
	}
}

/**
 * set_request_id_sync_reply
 * 同期reply待ちを開始する前にreq_idをとっておく
 */
static bool set_request_id_sync_reply (uint8_t thread_idx, uint32_t req_id)
{
	if (g_sync_reply_info [thread_idx].is_used) {
		/* 使われてる */
		return false;
	}

	g_sync_reply_info [thread_idx].is_used = true;
	g_sync_reply_info [thread_idx].req_id = req_id;
	return true;
}

/**
 * get_sync_reply_info
 * 同期reply待ち-> reply返って来て動き出した時に結果を取るため用(g_thm_src_infoにいれる)
 */
static sync_reply_info_t *get_sync_reply_info (uint8_t thread_idx)
{
	if (
		(!g_sync_reply_info [thread_idx].is_used) ||
		(g_sync_reply_info [thread_idx].req_id == REQUEST_ID_BLANK)
	) {
		/*
		 * set_request_id_sync_reply()を実行してない
		 * Reply待ちしてれば ここには来ないはず
		 */
		return NULL;
	}

	return &g_sync_reply_info [thread_idx];
}

/**
 * cache_sync_reply_info
 * 同期reply待ち中に replyする側が結果をキャッシュ messageも
 *
 * 引数 thread_idxは request元スレッドです
 */
static bool cache_sync_reply_info (uint8_t thread_idx, EN_THM_RSLT en_rslt, uint8_t *msg, size_t msg_size)
{
	if (
		(!g_sync_reply_info [thread_idx].is_used) ||
		(g_sync_reply_info [thread_idx].req_id == REQUEST_ID_BLANK)
	) {
		/*
		 * set_request_id_sync_reply()を実行してない
		 * Reply待ちしてれば ここには来ないはず
		 */
		return false;
	}

	/* result 保存 */
	g_sync_reply_info [thread_idx].en_rslt = en_rslt;

	/* message 保存 */
	if (msg && msg_size > 0) {
		if (msg_size > MSG_SIZE) {
			THM_INNER_FORCE_LOG_W ("truncate request message. size:[%lu]->[%d]\n", msg_size, MSG_SIZE);
		}
		memcpy (g_sync_reply_info [thread_idx].msg.msg, msg, msg_size < MSG_SIZE ? msg_size : MSG_SIZE);
		g_sync_reply_info [thread_idx].msg.size = msg_size < MSG_SIZE ? msg_size : MSG_SIZE;
		g_sync_reply_info [thread_idx].msg.is_used = true;
	}

	return true;
}

/**
 * set_reply_already_sync_reply_info
 *
 * 引数 thread_idxは request元スレッドです
 */
static void set_reply_already_sync_reply_info (uint8_t thread_idx)
{
	if (
		(!g_sync_reply_info [thread_idx].is_used) ||
		(g_sync_reply_info [thread_idx].req_id == REQUEST_ID_BLANK)
	) {
		/*
		 * set_request_id_sync_reply()を実行してない
		 * Reply待ちしてれば ここには来ないはず
		 */
		return;
	}

	g_sync_reply_info [thread_idx].is_reply_already = true;
}

/**
 * clear_sync_reply_info
 */
static void clear_sync_reply_info (sync_reply_info_t *p)
{
	if (!p) {
		return;
	}

	p->req_id = REQUEST_ID_BLANK;
	p->en_rslt = EN_THM_RSLT_IGNORE;
//TODO 暫定回避
// 受け渡し先がポインタなため 同期待受の最後で
// これを呼ぶところでmsgが消えてしまう
//	memset (p->msg.msg, 0x00, MSG_SIZE);
	p->msg.size = 0;
	p->msg.is_used = false;
	p->is_reply_already = false;
	p->is_used = false;
}

/**
 * Notify登録
 * 引数 p_client_id はout
 *
 * これを実行するコンテキストは登録先スレッドです(server側)
 * replyでclient_idを返してください
 * 複数クライアントからの登録は個々のスレッドでid管理しなくていはならない
 */
static bool register_notify (uint8_t category, uint8_t *pclient_id)
{
	/*
	 * get_context->自分のthread_idx取得->inner_infoを参照して登録先を得る
	 */

	if (category >= NOTIFY_CATEGORY_MAX) {
		THM_INNER_LOG_E ("invalid argument.\n");
		return false;
	}

	if (!pclient_id) {
		THM_INNER_LOG_E( "invalid argument.\n" );
		return false;
	}

	context_t ctx = get_context();
	if (!ctx.is_valid) {
		return false;
	}

	bool is_valid = get_seq_info (ctx.thread_idx, ctx.seq_idx)->seq_init_que_worker.is_valid_src_info;
	if (!is_valid) {
		/* 登録先がわからない */
		THM_INNER_LOG_E( "client is unknown.\n" );
		return false;
	}

	uint8_t client_thread_idx = get_seq_info (ctx.thread_idx, ctx.seq_idx)->seq_init_que_worker.src_thread_idx;


	uint8_t id = set_notify_client_info (ctx.thread_idx, category, client_thread_idx);
	if (id == NOTIFY_CLIENT_ID_MAX) {
		return false;
	}

	/* client_id 返す */
	if (pclient_id) {
		*pclient_id = id;
	}

	return true;
}

/**
 * Notify登録解除
 *
 * これを実行するコンテキストは登録先スレッドです(server側)
 * requestのmsgでclient_idをもらうこと
 */
static bool unregister_notify (uint8_t category, uint8_t client_id)
{
	if (client_id >= NOTIFY_CLIENT_ID_MAX) {
		THM_INNER_LOG_E( "invalid argument.\n" );
		return false;
	}

	context_t ctx = get_context();
	if (!ctx.is_valid) {
		return false;
	}

	return unset_notify_client_info (ctx.thread_idx, category, client_id);
}

/**
 * set_notify_client_info
 * セットしてクライアントIDを返す
 * NOTIFY_CLIENT_ID_MAX が返ったら空きがない状態 error
 */
static uint8_t set_notify_client_info (uint8_t thread_idx, uint8_t category, uint8_t client_thread_idx)
{
	uint8_t id = 0;

	/* lock */
//	pthread_mutex_lock (&g_mutex_notify_client_info [thread_idx]);
	pthread_mutex_lock (&g_mutex_notify_client_info);


	/* 空きを探す */
	for (id = 0; id < NOTIFY_CLIENT_ID_MAX; id ++) {
		if (!g_notify_client_info [id].is_used) {
			break;
		}
	}

	if (id == NOTIFY_CLIENT_ID_MAX) {
		/* 空きがない */
		THM_INNER_LOG_E ("client_id is full.\n");

		/* unlock */
//		pthread_mutex_unlock (&g_mutex_notify_client_info [thread_idx]);
		pthread_mutex_unlock (&g_mutex_notify_client_info);

		return id;
	}

	g_notify_client_info [id].src_thread_idx = thread_idx;
	g_notify_client_info [id].dest_thread_idx = client_thread_idx;
	g_notify_client_info [id].category = category;
	g_notify_client_info [id].is_used = true;


	/* unlock */
//	pthread_mutex_unlock (&g_mutex_notify_client_info [thread_idx]);
	pthread_mutex_unlock (&g_mutex_notify_client_info);

	return id;
}

/**
 * unset_notify_client_info
 */
static bool unset_notify_client_info (uint8_t thread_idx, uint8_t category, uint8_t client_id)
{
// 引数 category使用しない

	/* lock */
//	pthread_mutex_lock (&g_mutex_notify_client_info [thread_idx]);
	pthread_mutex_lock (&g_mutex_notify_client_info);

	if (!g_notify_client_info [client_id].is_used) {
		THM_INNER_LOG_E ("client_id is not use...?");

//		pthread_mutex_unlock (&g_mutex_notify_client_info [thread_idx]);
		pthread_mutex_unlock (&g_mutex_notify_client_info);

		return false;
	}

	clear_notify_client_info (&g_notify_client_info [client_id]);

	/* unlock */
//	pthread_mutex_unlock (&g_mutex_notify_client_info [thread_idx]);
	pthread_mutex_unlock (&g_mutex_notify_client_info);

	return true;
}

/**
 * notify_inner
 *
 * 引数 thread_idx は宛先です
 */
static bool notify_inner (
	uint8_t thread_idx,
	uint8_t client_id,
	context_t *context,
	uint8_t *msg,
	size_t msg_size
)
{
	/* lock */
	pthread_mutex_lock (&g_mutex_worker [thread_idx]);

	/* キューに入れる */
	if (!enque_worker (thread_idx, SEQ_IDX_BLANK, EN_QUE_TYPE_NOTIFY,
							context, REQUEST_ID_BLANK, EN_THM_RSLT_IGNORE, client_id, msg, msg_size)) {
		/* unlock */
		pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

		THM_INNER_LOG_E ("en_que_worker() is failure.\n");
		return false;
	}


	/*
	 * Notify
	 * cond signal ---> worker_thread
	 */
	pthread_cond_signal (&g_cond_worker [thread_idx]);


	/* unlock */
	pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

	return true;
}

/**
 * Notify送信
 * 公開
 */
static bool notify (uint8_t category, uint8_t *msg, size_t msg_size)
{
	/*
	 * get_context->自分のthread_idx取得-> notify_client_infoをみて trueだったらenque
	 */

	if (category >= NOTIFY_CATEGORY_MAX) {
		THM_INNER_LOG_E ("invalid argument.\n");
		return false;
	}

	context_t ctx = get_context();
	if (!ctx.is_valid) {
		return false;
	}


	uint8_t client_id = 0;
	/* 自分のthread_idxの該当categoryに属したclientにNotify投げる */
	for (client_id = 0; client_id < NOTIFY_CLIENT_ID_MAX; ++ client_id) {

		if (!g_notify_client_info [client_id].is_used) {
			continue;
		}

		if (g_notify_client_info [client_id].src_thread_idx != ctx.thread_idx) {
			continue;
		}

		if (g_notify_client_info [client_id].category != category) {
			continue;
		}

		uint8_t client_thread_idx = g_notify_client_info [client_id].dest_thread_idx;


		if (!notify_inner (client_thread_idx, client_id, &ctx, msg, msg_size)) {
			THM_INNER_LOG_E (
				"notify_inner failure. client_thread_idx=[0x%02x] category=[0x%02x] client_id=[0x%02x]\n",
				client_thread_idx,
				category,
				client_id
			);
		}
	}

	return true;
}

/**
 * dump_notify_client_info
 */
static void dump_notify_client_info (void)
{
	uint32_t i = 0;

//TODO 参照だけ ログだけだからmutexしない

	THM_LOG_I ("####  dump_notify_client_info  ####\n");

	for (i = 0; i < NOTIFY_CLIENT_ID_MAX; ++ i) {
		if (!g_notify_client_info [i].is_used) {
			continue;
		}

		THM_LOG_I (
			" %d: server:[0x%02x][%-15s] client:[0x%02x][%-15s] category:[0x%02x]\n",
			i,
			g_notify_client_info [i].src_thread_idx,
			g_inner_info [g_notify_client_info [i].src_thread_idx].name,
			g_notify_client_info [i].dest_thread_idx,
			g_inner_info [g_notify_client_info [i].dest_thread_idx].name,
			g_notify_client_info [i].category
		);
	}
}

/**
 * clear_notify_client_info
 */
static void clear_notify_client_info (notify_client_info_t *p)
{
	if (!p) {
		return;
	}

	p->src_thread_idx = THREAD_IDX_BLANK;
	p->dest_thread_idx = THREAD_IDX_BLANK;
	p->category = NOTIFY_CATEGORY_BLANK;
	p->is_used = false;
}

/**
 * set_sect_id
 * 次のsect_idをセットする
 * 公開用
 */
static void set_sect_id (uint8_t n_sect_id, EN_THM_ACT en_act)
{
	if ((n_sect_id < SECT_ID_INIT) || (n_sect_id >= SECT_ID_MAX)) {
		THM_INNER_LOG_E ("arg(sect_id) is invalid.\n");
		return;
	}

	if ((en_act < EN_THM_ACT_INIT) || (en_act >= EN_THM_ACT_MAX)) {
		THM_INNER_LOG_E ("arg(en_act) is invalid.\n");
		return;
	}

	if (en_act == EN_THM_ACT_INIT) {
		/* ユーザ自身でEN_THM_ACT_INITに設定はできない */
		THM_INNER_LOG_E ("set EN_THM_ACT_INIT can not on their own.\n");
		return;
	}

	if (en_act == EN_THM_ACT_DONE) {
		en_act = EN_THM_ACT_INIT;
	}

	context_t ctx = get_context();
	if (ctx.is_valid) {
		set_sect_id_inner (ctx.thread_idx, ctx.seq_idx, n_sect_id, en_act);
	}
}

/**
 * set_sect_id_inner
 */
static void set_sect_id_inner (uint8_t thread_idx, uint8_t seq_idx, uint8_t n_sect_id, EN_THM_ACT en_act)
{
	get_seq_info (thread_idx, seq_idx)->sect_id = n_sect_id;
	get_seq_info (thread_idx, seq_idx)->en_act = en_act;
}

/**
 * clear_sect_id
 */
static void clear_sect_id (uint8_t thread_idx, uint8_t seq_idx)
{
	set_sect_id_inner (thread_idx, seq_idx, SECT_ID_INIT, EN_THM_ACT_INIT);
}

/**
 * get_sect_id
 * 公開用
 */
static uint8_t get_sect_id (void)
{
	context_t ctx = get_context();
	if (ctx.is_valid) {
		if (get_seq_info (ctx.thread_idx, ctx.seq_idx)->en_act == EN_THM_ACT_INIT) {
			get_seq_info (ctx.thread_idx, ctx.seq_idx)->sect_id = SECT_ID_INIT;
			return SECT_ID_INIT;
		} else {
			return get_seq_info (ctx.thread_idx, ctx.seq_idx)->sect_id;
		}
	} else {
		return SECT_ID_INIT;
	}
}

/**
 * get_seq_idx
 * 公開用
 */
static uint8_t get_seq_idx (void)
{
	context_t ctx = get_context();
	if (ctx.is_valid) {
		return ctx.seq_idx;
	} else {
		return SEQ_IDX_BLANK;
	}
}

/**
 * get_seq_name
 * 公開用
 */
static const char* get_seq_name (void)
{
	context_t ctx = get_context();
	if (ctx.is_valid) {
		const threadmgr_seq_t *p = gp_thm_reg_tbl [ctx.thread_idx]->seq_array;
		return (p + ctx.seq_idx)->name;
	} else {
		return NULL;
	}
}

/**
 * set_timeout
 * 公開用
 * 登録シーケンスがreturn してから時間計測開始します
 * クリアされるタイミングは 実際にタイムアウトした時と ユーザ自身iがclear_timeoutした時です
 */
static void set_timeout (uint32_t timeout_msec)
{
	set_seq_timeout (timeout_msec);
}

/**
 * set_seq_timeout
 */
static void set_seq_timeout (uint32_t timeout_msec)
{
	if ((timeout_msec < SEQ_TIMEOUT_BLANK) || (timeout_msec >= SEQ_TIMEOUT_MAX)) {
		THM_INNER_LOG_E ("arg is invalid.\n");
		return;
	}

	context_t ctx = get_context();
	if (ctx.is_valid) {
		uint8_t seq_idx = ctx.seq_idx;
		/* ここでは値のみ */
		get_seq_info (ctx.thread_idx, seq_idx)->timeout.val = timeout_msec;
	}
}

/**
 * enable_seq_timeout
 * Seqタイムアウトを仕掛ける
 */
static void enable_seq_timeout (uint8_t thread_idx, uint8_t seq_idx)
{
	if (get_seq_info (thread_idx, seq_idx)->timeout.en_state != EN_TIMEOUT_STATE_INIT) {
		/*
		 * set_timeoutしてsectionまたいでWAITしている時とかは
		 * 既に設定済なので何もしない よって上書きされない
		 * 1seq内では seq_timeout1つのみ設定可能です
		 */
		return;
	}

	uint32_t n_timeout = get_seq_info (thread_idx, seq_idx)->timeout.val;
	time_t add_sec = n_timeout / 1000;
	long add_nsec  = (n_timeout % 1000) * 1000000;
	THM_INNER_LOG_D ("add_sec:%ld  add_nsec:%ld\n", add_sec, add_nsec);

	struct timeval now_timeval = {0};
	get_time_of_day (&now_timeval);
	time_t now_sec = now_timeval.tv_sec;
	long now_nsec  = now_timeval.tv_usec * 1000;

	get_seq_info (thread_idx, seq_idx)->timeout.time.tv_sec = now_sec + add_sec + ((now_nsec + add_nsec) / 1000000000);
	get_seq_info (thread_idx, seq_idx)->timeout.time.tv_nsec = (now_nsec + add_nsec) % 1000000000;
	get_seq_info (thread_idx, seq_idx)->timeout.en_state = EN_TIMEOUT_STATE_MEAS;
	THM_INNER_LOG_D (
		"timeout.time.tv_sec:%ld  timeout.time.tv_nsec:%ld\n",
		get_seq_info (thread_idx, seq_idx)->timeout.time.tv_sec,
		get_seq_info (thread_idx, seq_idx)->timeout.time.tv_nsec
	);
}

/**
 * check_seq_timeout
 * タイムアウトしたかどうかの確認して キュー入れる
 * 
 * 一連の登録シーケンス実行後にチェック
 */
static void check_seq_timeout (uint8_t thread_idx)
{
	uint8_t nr_seq = g_inner_info [thread_idx].nr_seq;
	int i = 0;
	for (i = 0; i < nr_seq; ++ i) {
		/* EN_TIMEOUT_STATE_PASSEDの場合はすでにqueuingされてるはず */
		if (get_seq_info (thread_idx, i)->timeout.en_state == EN_TIMEOUT_STATE_MEAS) {
			if (is_seq_timeout_from_seq_idx (thread_idx, i)) {
				/* 自スレッドにSeqタイムアウトのキューを入れる */
				get_seq_info (thread_idx, i)->timeout.en_state = EN_TIMEOUT_STATE_PASSED;
				enque_seq_timeout (thread_idx, i);
			}
		}
	}
}

/**
 * is_seq_timeout_from_seq_idx
 */
static bool is_seq_timeout_from_seq_idx (uint8_t thread_idx, uint8_t seq_idx)
{
	bool is_timeout = false;
	struct timeval now_timeval = {0};
	get_time_of_day (&now_timeval);
	time_t now_sec = now_timeval.tv_sec;
	long now_nsec  = now_timeval.tv_usec * 1000;

	time_t timeout_sec = get_seq_info (thread_idx, seq_idx)->timeout.time.tv_sec;
	long timeout_nsec  = get_seq_info (thread_idx, seq_idx)->timeout.time.tv_nsec;

	time_t diff_sec = 0;
	long diff_nsec = timeout_nsec - now_nsec;
	if (diff_nsec >= 0) {
		diff_sec = timeout_sec - now_sec;
	} else {
		diff_sec = timeout_sec -1 - now_sec;
		diff_nsec += 1000000000;
	}

	if (diff_sec > 0) {
		/* まだタイムアウトしてない */
		THM_INNER_LOG_I ("not yet timeout(seq_timeout) thread_idx:[%d] seq_idx:[%d]\n", thread_idx, seq_idx);

	} else if (diff_sec == 0) {
		if (diff_nsec > 0) {
			/* まだタイムアウトしてない 1秒切ってる */
			THM_INNER_LOG_I ("not yet timeout - off the one second (seq_timeout) thread_idx:[%d] seq_idx:[%d]\n", thread_idx, seq_idx);

		} else if (diff_nsec == 0) {
			/* ちょうどタイムアウト かなりレア... */
			THM_INNER_LOG_I ("just timedout(seq_timeout) thread_idx:[%d] seq_idx:[%d]\n", thread_idx, seq_idx);
			is_timeout = true;

		} else {
			/* ありえない */
			THM_INNER_LOG_E ("Unexpected: diff_nsec is minus value.\n");
		}

	} else {
		/* 既にタイムアウトしてる... */
		THM_INNER_LOG_I ("already timedout(seq_timeout) thread_idx:[%d] seq_idx:[%d]\n", thread_idx, seq_idx);
		is_timeout = true;
	}

	return is_timeout;
}

/**
 * en_que_seq_timeout
 */
static bool enque_seq_timeout (uint8_t thread_idx, uint8_t seq_idx)
{
	context_t ctx = get_context();

	if (!enque_worker(thread_idx, seq_idx, EN_QUE_TYPE_SEQ_TIMEOUT,
							&ctx, REQUEST_ID_BLANK, EN_THM_RSLT_SEQ_TIMEOUT, NOTIFY_CLIENT_ID_BLANK, NULL, 0)) {
		THM_INNER_LOG_E ("en_que_worker() is failure.\n");
		return false;
	}

	return true;
}

/**
 * search_nearest_seq_timeout
 * Seqタイムアウト仕掛り中のもので 最も近い時間のものを探す
 * seq_info_t で返します
 * タイムアウト仕掛けてなければnullを返します
 *
 * cond_waitする直前にチェックします
 * pthread_cond_wait か pthread_cond_timedwaitにするかの判断
 */
static seq_info_t *search_nearest_seq_timeout (uint8_t thread_idx)
{
	time_t timeout_sec_min = 0;
	time_t timeout_nsec_min = 0;
	time_t timeout_sec = 0;
	long timeout_nsec  = 0;
	uint8_t n_rtseq_idx = SEQ_IDX_MAX;

	uint8_t nr_seq = g_inner_info [thread_idx].nr_seq;
	int i = 0;
	for (i = 0; i < nr_seq; ++ i) {
		if (get_seq_info (thread_idx, i)->timeout.en_state == EN_TIMEOUT_STATE_MEAS) {

			if ((timeout_sec_min == 0) && (timeout_nsec_min == 0)) {
				// 初回
				timeout_sec_min = get_seq_info (thread_idx, i)->timeout.time.tv_sec;
				timeout_nsec_min = get_seq_info (thread_idx, i)->timeout.time.tv_nsec;
				n_rtseq_idx = i;

			} else {
				timeout_sec = get_seq_info (thread_idx, i)->timeout.time.tv_sec;
				timeout_nsec = get_seq_info (thread_idx, i)->timeout.time.tv_nsec;

				if (timeout_sec < timeout_sec_min) {
					timeout_sec_min = timeout_sec;
					timeout_nsec_min = timeout_nsec;
					n_rtseq_idx = i;

				} else if (timeout_sec == timeout_sec_min) {
					if (timeout_nsec < timeout_nsec_min) {
						timeout_sec_min = timeout_sec;
						timeout_nsec_min = timeout_nsec;
						n_rtseq_idx = i;
					}
				}
			}
		}
	}

	if (n_rtseq_idx == SEQ_IDX_MAX) {
		/* タイムアウト仕掛かけてない */
		return NULL;
	} else {
		return get_seq_info (thread_idx, n_rtseq_idx);
	}
}

/**
 * check_seq_timeout_from_cond_timedwait
 * EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAITの状態をみつけてキューを入れる
 *
 * pthread_cond_timedwaitからタイムアウトで戻った時に使います
 */
static void check_seq_timeout_from_cond_timedwait (uint8_t thread_idx)
{
	uint8_t nr_seq = g_inner_info [thread_idx].nr_seq;
	int i = 0;
	int n = 0;
	for (i = 0; i < nr_seq; ++ i) {
		if (get_seq_info (thread_idx, i)->timeout.en_state == EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT) {
			++ n;
		}
	}

	if (n != 1) {
		/* 	複数存在した... バグ */
		THM_INNER_LOG_E ("Unexpected: state(EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT) is multiple. n=[%d]\n", n);
		return;
	}

	/* 自スレッドにSeqタイムアウトのキューを入れる */
	enque_seq_timeout (thread_idx, i);
}

/**
 * check_seq_timeout_from_not_cond_timedwait
 * pthread_cond_timedwaitから戻った時 タイムアウトでない場合
 * 状態をEN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT ->  EN_TIMEOUT_STATE_MEASに変える
 */
static void check_seq_timeout_from_not_cond_timedwait (uint8_t thread_idx)
{
	uint8_t nr_seq = g_inner_info [thread_idx].nr_seq;
	int i = 0;
	int n = 0;
	for (i = 0; i < nr_seq; ++ i) {
		if (get_seq_info (thread_idx, i)->timeout.en_state == EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT) {
			get_seq_info (thread_idx, i)->timeout.en_state = EN_TIMEOUT_STATE_MEAS;
			++ n;
		}
	}

	if (n != 1) {
		/* 	複数存在した... バグ */
		THM_INNER_LOG_E ("Unexpected: state(EN_TIMEOUT_STATE_MEAS_COND_TIMEDWAIT) is multiple. n=[%d]\n", n);
		return;
	}
}

/**
 * clear_timeout
 * 公開用
 */
static void clear_timeout (void)
{
	context_t ctx = get_context();
	if (ctx.is_valid) {
		clear_seq_timeout (ctx.thread_idx, ctx.seq_idx);
	}
}

/**
 * clear_seq_timeout
 */
static void clear_seq_timeout (uint8_t thread_idx, uint8_t seq_idx)
{
	seq_info_t *p_seq_info = get_seq_info (thread_idx, seq_idx);
	if (!p_seq_info) {
		return;
	}

	p_seq_info->timeout.en_state = EN_TIMEOUT_STATE_INIT;
	p_seq_info->timeout.val = SEQ_TIMEOUT_BLANK;
	memset (&(p_seq_info->timeout.time), 0x00, sizeof(struct timespec));
}

/**
 * get_seq_info
 *
 * seq_infoは同じスレッドからしか操作しないので特に問題ないはず
 */
static seq_info_t *get_seq_info (uint8_t thread_idx, uint8_t seq_idx)
{
	return (g_inner_info [thread_idx].seq_info)+seq_idx;
}

/**
 * clear_seq_info
 */
static void clear_seq_info (seq_info_t *p)
{
	if (!p) {
		return;
	}

	p->seq_idx = SEQ_IDX_BLANK;

	p->sect_id = SECT_ID_INIT;
	p->en_act = EN_THM_ACT_INIT;
#ifndef _MULTI_REQUESTING
	p->req_id = REQUEST_ID_BLANK;
#endif
	clear_que_worker (&(p->seq_init_que_worker));
	p->is_overwrite = false;
	p->is_lock = false;

	p->timeout.en_state = EN_TIMEOUT_STATE_INIT;
	p->timeout.val = SEQ_TIMEOUT_BLANK;
	memset (&(p->timeout.time), 0x00, sizeof(struct timespec));

	p->running_sect_id = SEQ_IDX_BLANK;
}

/**
 * enable_overwrite
 * 公開用
 */
static void enable_overwrite (void)
{
	context_t ctx = get_context();
	if (ctx.is_valid) {
		set_overwrite (ctx.thread_idx, ctx.seq_idx, true);
	}
}

/**
 * disable_overwrite
 * 公開用
 */
static void disable_overwrite (void)
{
	context_t ctx = get_context();
	if (ctx.is_valid) {
		set_overwrite (ctx.thread_idx, ctx.seq_idx, false);
	}
}

/**
 * set_overwrite
 */
static void set_overwrite (uint8_t thread_idx, uint8_t seq_idx, bool is_overwrite)
{
	seq_info_t *p_seq_info = get_seq_info (thread_idx, seq_idx);
	if (!p_seq_info) {
		return;
	}

	p_seq_info->is_overwrite = is_overwrite;
}

/**
 * is_lock
 * 当該スレッドでlockしているseqが存在するかどうか
 */
static bool is_lock (uint8_t thread_idx)
{
	bool r = false;
	uint8_t nr_seq = g_inner_info [thread_idx].nr_seq;
	int i = 0;
	int n = 0;

	for (i = 0; i < nr_seq; ++ i) {
		if (get_seq_info (thread_idx, i)->is_lock) {
			r = true;
			++ n;
		}
	}

	if (n >= 2) {
		THM_INNER_LOG_E ("Unexpected: Multiple seq locked.\n");
	}

	return r;
}

/**
 * is_lock_seq
 * 引数seq_idxがlockしているかどうか
 */
static bool is_lock_seq (uint8_t thread_idx, uint8_t seq_idx)
{
	bool r = false;
	uint8_t nr_seq = g_inner_info [thread_idx].nr_seq;
	int i = 0;

	for (i = 0; i < nr_seq; ++ i) {
		if (get_seq_info (thread_idx, i)->is_lock) {
			if (i == seq_idx) {
				r = true;
			}
		}
	}
	
	return r;
}

/**
 * lock
 * 公開用
 */
static void lock (void)
{
	context_t ctx = get_context();
	if (ctx.is_valid) {
		set_lock (ctx.thread_idx, ctx.seq_idx, true);
	}
}

/**
 * unlock
 * 公開用
 */
static void unlock (void)
{
	context_t ctx = get_context();
	if (ctx.is_valid) {
		set_lock (ctx.thread_idx, ctx.seq_idx, false);
	}
}

/**
 * set_lock
 */
static void set_lock (uint8_t thread_idx, uint8_t seq_idx, bool is_lock)
{
	seq_info_t *p_seq_info = get_seq_info (thread_idx, seq_idx);
	if (!p_seq_info) {
		return;
	}

	p_seq_info->is_lock = is_lock;
}

/**
 * search_nearetimeout
 * ReqタイムアウトとSeqタイムアウト仕掛り中のもので 最も近い時間のものを探す
 * タイムアウト仕掛けてなければ EN_NEAREST_TIMEOUT_NONE を返します
 *
 * cond_waitする直前にチェックします
 * pthread_cond_wait か pthread_cond_timedwaitにするかの判断
 */
static EN_NEAREST_TIMEOUT search_nearetimeout (
	uint8_t thread_idx,
	request_id_info_t **p_request_id_info,	// out
	seq_info_t **p_seq_info		// out
)
{
	request_id_info_t *p_tmp_req_id_info = search_nearest_req_timeout (thread_idx);
	seq_info_t *p_tmp_seq_info = search_nearest_seq_timeout (thread_idx);
    time_t req_timeout_sec = 0;
    long req_timeout_nsec  = 0;
    time_t seq_timeout_sec = 0;
    long seq_timeout_nsec  = 0;


	*p_request_id_info = NULL;
	*p_seq_info = NULL;

	if (!p_tmp_req_id_info && !p_tmp_seq_info) {
		THM_INNER_LOG_D ("EN_NEAREST_TIMEOUT_NONE\n");
		return EN_NEAREST_TIMEOUT_NONE;

	} else if (p_tmp_req_id_info && !p_tmp_seq_info) {
		THM_INNER_LOG_D ("EN_NEAREST_TIMEOUT_REQ\n");
		*p_request_id_info = p_tmp_req_id_info;
		return EN_NEAREST_TIMEOUT_REQ;

	} else if (!p_tmp_req_id_info && p_tmp_seq_info) {
		THM_INNER_LOG_D ("EN_NEAREST_TIMEOUT_SEQ\n");
		*p_seq_info = p_tmp_seq_info;
		return EN_NEAREST_TIMEOUT_SEQ;

	} else {
		/* Req,Seqタイムアウト両方とも仕掛かっているので 値を比較します */

		req_timeout_sec = p_tmp_req_id_info->timeout.time.tv_sec;
		req_timeout_nsec = p_tmp_req_id_info->timeout.time.tv_nsec;
		seq_timeout_sec = p_tmp_seq_info->timeout.time.tv_sec;
		seq_timeout_nsec = p_tmp_seq_info->timeout.time.tv_nsec;

		if (req_timeout_sec < seq_timeout_sec) {
			THM_INNER_LOG_D ("both timeout enabled. -> EN_NEAREST_TIMEOUT_REQ\n");
			*p_request_id_info = p_tmp_req_id_info;
			return EN_NEAREST_TIMEOUT_REQ;

		} else if (req_timeout_sec > seq_timeout_sec) {
			THM_INNER_LOG_D ("both timeout enabled. -> EN_NEAREST_TIMEOUT_SEQ\n");
			*p_seq_info = p_tmp_seq_info;
			return EN_NEAREST_TIMEOUT_SEQ;

		} else {
			/* 整数値等しい場合 */

			if (req_timeout_nsec < seq_timeout_nsec) {
				THM_INNER_LOG_D ("both timeout enabled. -> EN_NEAREST_TIMEOUT_REQ (integer value equal)\n");
				*p_request_id_info = p_tmp_req_id_info;
				return EN_NEAREST_TIMEOUT_REQ;

			} else if (req_timeout_nsec > seq_timeout_nsec) {
				THM_INNER_LOG_D ("both timeout enabled. -> EN_NEAREST_TIMEOUT_SEQ (integer value equal)\n");
				*p_seq_info = p_tmp_seq_info;
				return EN_NEAREST_TIMEOUT_SEQ;

			} else {
				/* 小数点以下も同じ req_seq_timeoutを返しておく */
				THM_INNER_LOG_D ("both timeout enabled. -> equal !!!\n");
				*p_request_id_info = p_tmp_req_id_info;
				return EN_NEAREST_TIMEOUT_REQ;
			}
		}

	}
}

/**
 * add_ext_info_list
 * pthread_tをキーとしたリスト
 * キーはuniqなものとして扱う前提 (1threadにつき1つのデータをもつ)
 */
static void add_ext_info_list (external_control_info_t *ext_info)
{
	/* lock */
	pthread_mutex_lock (&g_mutex_ope_ext_info_list);

	if (!g_ext_info_list_top) {
		// リストが空
		g_ext_info_list_top = ext_info;
		g_ext_info_list_btm = ext_info;

	} else {
		// 末尾につなぎます
		g_ext_info_list_btm->next = ext_info;
		g_ext_info_list_btm = ext_info;
	}

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_ext_info_list);
}

/**
 * search_ext_info_list
 */
static external_control_info_t *search_ext_info_list (pthread_t key)
{
	external_control_info_t *ext_info_tmp = NULL;

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_ext_info_list);

	if (!g_ext_info_list_top) {
		// リストが空

	} else {
		ext_info_tmp = g_ext_info_list_top;

		while (ext_info_tmp) {
			if (pthread_equal (ext_info_tmp->pthread_id, key)) {
				// keyが一致
				break;
			}

			// next set
			ext_info_tmp = ext_info_tmp->next;
		}
	}

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_ext_info_list);

	return ext_info_tmp;
}

/**
 * search_ext_info_list_from_request_id
 */
static external_control_info_t *search_ext_info_list_from_request_id (uint32_t req_id)
{
	external_control_info_t *ext_info_tmp = NULL;

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_ext_info_list);

	if (!g_ext_info_list_top) {
		// リストが空

	} else {
		ext_info_tmp = g_ext_info_list_top;

		while (ext_info_tmp) {
			if (ext_info_tmp->req_id == req_id) {
				// req_idが一致
				break;
			}

			// next set
			ext_info_tmp = ext_info_tmp->next;
		}
	}

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_ext_info_list);

	return ext_info_tmp;
}

/**
 * delete_ext_info_list
 */
static void delete_ext_info_list (pthread_t key)
{
	external_control_info_t *ext_info_tmp = NULL;
	external_control_info_t *ext_info_bef = NULL;
	external_control_info_t *ext_info_del = NULL;

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_ext_info_list);

	if (!g_ext_info_list_top) {
		// リストが空

	} else {
		ext_info_tmp = g_ext_info_list_top;

		while (ext_info_tmp) {
			if (pthread_equal (ext_info_tmp->pthread_id, key)) {
				// keyが一致

				// 後でfreeするため保存
				ext_info_del = ext_info_tmp;

				if (ext_info_tmp == g_ext_info_list_top) {
					// topの場合

					if (ext_info_tmp->next) {
						g_ext_info_list_top = ext_info_tmp->next;
						ext_info_tmp->next = NULL;
					} else {
						// リストが１つ -> リストが空になるはず
						g_ext_info_list_top = NULL;
						g_ext_info_list_btm = NULL;
						ext_info_tmp->next = NULL;
					}

				} else if (ext_info_tmp == g_ext_info_list_btm) {
					// bottomの場合

					if (ext_info_bef) {
						// 切り離します
						ext_info_bef->next = NULL;
						g_ext_info_list_btm = ext_info_bef;
					}

				} else {
					// 切り離します
					ext_info_bef->next = ext_info_tmp->next;
					ext_info_tmp->next = NULL;
				}

				break;

			} else {
				// ひとつ前のアドレス保存
				ext_info_bef = ext_info_tmp;
			}

			// next set
			ext_info_tmp = ext_info_tmp->next;
		}

		free (ext_info_del);
	}

	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_ext_info_list);
}

/**
 * dump_ext_info_list
 * デバッグ用
 */
static void dump_ext_info_list (void)
{
	external_control_info_t *ext_info_tmp = NULL;
	int n = 0;

	/* lock */
	pthread_mutex_lock (&g_mutex_ope_ext_info_list);

	THM_LOG_I ("####  dump external_info_list  ####\n");

	ext_info_tmp = g_ext_info_list_top;
	while (ext_info_tmp) {

		THM_LOG_I (" %d: %lu req_id:[0x%x]  (%p -> %p)\n", n, ext_info_tmp->pthread_id, ext_info_tmp->req_id, ext_info_tmp, ext_info_tmp->next);

		// next set
		ext_info_tmp = ext_info_tmp->next;
		++ n;
	}


	/* unlock */
	pthread_mutex_unlock (&g_mutex_ope_ext_info_list);
}

/**
 * create_external_cp
 * 外部のスレッドからリクエストしてリプライを受けたい場合は これをリクエスト前に実行しておく
 * 1thread につき 1つ内部情報を保持します
 *
 * 公開 external_if
 */
bool create_external_cp (void)
{
	/* 内部でよばれたら実行しないチェック */
	context_t ctx = get_context();
	if (ctx.is_valid) {
		return false;
	}


	external_control_info_t *p_tmp = search_ext_info_list(pthread_self());
	if (p_tmp) {
		/* このスレッドではすでに登録されている 古いものは消します */
		delete_ext_info_list (pthread_self());
	}

	external_control_info_t *ext_info = (external_control_info_t*) malloc (sizeof(external_control_info_t));
	if (!ext_info) {
		return false;
	}
	clear_external_control_info (ext_info);

	add_ext_info_list (ext_info);


	return true;
}

/**
 * destroy_external_cp
 * 公開 external_if
 */
void destroy_external_cp (void)
{
	/* 内部でよばれたら実行しないチェック */
	context_t ctx = get_context();
	if (ctx.is_valid) {
		return;
	}


	delete_ext_info_list (pthread_self());
}

/**
 * receive_external
 * 外部のスレッドからリクエストした場合はこれでリプライを待ち 結果をとります
 *
 * 公開 external_if
 */
threadmgr_src_info_t *receive_external (void)
{
	/* 内部でよばれたら実行しないチェック */
	context_t ctx = get_context();
	if (ctx.is_valid) {
		return NULL;
	}


	external_control_info_t *ext_info = search_ext_info_list (pthread_self());
	if (!ext_info) {
		return NULL;
	}

	/* lock */
	pthread_mutex_lock (&(ext_info->mutex));

	/* is_reply_alreadyチェックして cond で待つかどうか判断 */
	if (!ext_info->is_reply_already) {
		/* 相手がまだreplyしてないので cond wait */
		pthread_cond_wait (&(ext_info->cond), &(ext_info->mutex));
	}

	ext_info->is_reply_already = false;

	/* req_id解放して ext_infoのreq_idもクリア */
	release_request_id (THREAD_IDX_EXTERNAL, ext_info->req_id);
	ext_info->req_id = REQUEST_ID_BLANK;

	/* unlock */
	pthread_mutex_unlock (&(ext_info->mutex));


	return &(ext_info->thm_src_info);
}

/**
 * clear_external_control_info
 */
static void clear_external_control_info (external_control_info_t *p)
{
	if (!p) {
		return;
	}

	pthread_mutex_init (&(p->mutex), NULL);
	pthread_cond_init (&(p->cond), NULL);

	p->pthread_id = pthread_self();
	p->req_id = REQUEST_ID_BLANK;

	memset (p->msg_entity.msg, 0x00, MSG_SIZE);
	p->msg_entity.size = 0;
	clear_thm_src_info (&(p->thm_src_info));

	p->is_reply_already = false;

    p->request_option = 0;
    p->requetimeout_msec = 0;

	p->next = NULL;
}

/**
 * finaliz
 * 終了処理
 *
 * 公開
 */
void finaliz (void)
{
	kill (getpid(), SIGTERM);
	wait_all ();
	finaliz_sem();
//	finaliz_signal();
}

/**
 * destroy_inner
 *
 * 引数 thread_idx は宛先です
 */
static bool destroy_inner (uint8_t thread_idx)
{
	/* lock */
	pthread_mutex_lock (&g_mutex_worker [thread_idx]);

	/* キューに入れる */
	if (!enque_worker (thread_idx, SEQ_IDX_BLANK, EN_QUE_TYPE_DESTROY,
						NULL, REQUEST_ID_BLANK, EN_THM_RSLT_IGNORE, NOTIFY_CLIENT_ID_BLANK, NULL, 0)) {
		/* unlock */
		pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

		THM_INNER_LOG_E ("en_que_worker() is failure.\n");
		return false;
	}


	/*
	 * Notify
	 * cond signal ---> worker_thread
	 */
	pthread_cond_signal (&g_cond_worker [thread_idx]);


	/* unlock */
	pthread_mutex_unlock (&g_mutex_worker [thread_idx]);

	return true;
}

/**
 * destroy_worker_thread
 * worker_thread破棄 async
 */
static bool destroy_worker_thread (uint8_t thread_idx)
{
	if (!destroy_inner(thread_idx)) {
		return false;
	}

	return true;
}

/**
 * create_all_thread
 * 全workerスレッド破棄 async
 */
static bool destroy_all_worker_thread (void)
{
	uint8_t i = 0;

	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		if (!destroy_worker_thread (i)) {
			return false;
		}
		THM_INNER_FORCE_LOG_I ("destroy worker_thread. th_idx:[%d]\n", i);
	}

	return true;
}

/**
 * wait_all
 * busy wait
 *
 * 公開
 */
void wait_all (void)
{
	uint8_t i = 0;

	for (i = 0; i < get_total_worker_thread_num(); ++ i) {
		wait_worker_thread (i);
	}

	wait_base_thread();
}

/**
 * wait_base_thread
 */
static void wait_base_thread (void)
{
	while (1) {
		if (!is_exist_thread(g_tid_sig_wait_thread)) {
			break;
		}
		sleep (1);
	}

	while (1) {
		if (!is_exist_thread(g_tid_base_thread)) {
			break;
		}
		sleep (1);
	}
}

/**
 * wait_worker_thread
 */
static void wait_worker_thread (uint8_t thread_idx)
{
	while (1) {
		if (!is_exist_thread (g_inner_info [thread_idx].tid)) {
			break;
		}
		sleep (1);
	}
}

/**
 * is_enable_log
 */
static bool is_enable_log (void)
{
	return g_is_enable_log;
}

/**
 * enable_log
 */
static void enable_log (void)
{
	g_is_enable_log = true;
}

/**
 * disable_log
 */
static void disable_log (void)
{
	g_is_enable_log = false;
}

/**
 * set_dispatcher
 * for c++ wrapper extension
 */
void set_dispatcher (const PFN_DISPATCHER pfn_dispatcher)
{
	if (pfn_dispatcher) {
		gpfn_dispatcher = pfn_dispatcher;
	}
}
