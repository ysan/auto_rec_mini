/*
 * coseq_if.h --- coseq 公開C API (fiber ネイティブ / インスタンス形式)
 *
 *   - section_id / action は無い。シーケンスは一直線に書く。
 *   - グローバル状態を持たない。create_coseq() が返すインスタンスハンドル
 *     (coseq_ctx_if_t*) 経由で setup/teardown/外部request を行う。
 *     → 1プロセスに複数の coseq インスタンスを持てる。
 *   - シーケンス内API(coseq_request 等)は p_if を受け取り、内部でその p_if が
 *     属するインスタンスに到達するので、利用側の記述は変わらない。
 */
#ifndef _COSEQ_IF_H_
#define _COSEQ_IF_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 結果コード */
typedef enum {
	COSEQ_RSLT_IGNORE = 0,
	COSEQ_RSLT_SUCCESS,
	COSEQ_RSLT_ERROR,
	COSEQ_RSLT_REQ_TIMEOUT,
	COSEQ_RSLT_SEQ_TIMEOUT,
} coseq_result_t;

/*
 * --- ログ ---
 *   debug / info / warn / error の4段。ログはコールバックで注入(既定=何もしない)。
 *   - debug : 毎イベントの処理トレース(request/reply/notify/fiber 等)。既定では除去。
 *   - info  : ライフサイクル(setup/teardown/scheduler 起動停止)。
 *   - warn / error : 異常。
 *   レベルは #if で比較できるようマクロ。COSEQ_LOG_MIN 未満はコンパイル時に完全除去される。
 *   既定は INFO(=DEBUG は除去)。全トレースを見るなら -DCOSEQ_LOG_MIN=COSEQ_LOG_DEBUG、
 *   release で info も消すなら -DCOSEQ_LOG_MIN=COSEQ_LOG_WARN。
 */
/* info/warn/error は 0/1/2 で固定(公開後に変えない)。debug は下位なので -1。 */
#define COSEQ_LOG_DEBUG  (-1)
#define COSEQ_LOG_INFO   0
#define COSEQ_LOG_WARN   1
#define COSEQ_LOG_ERROR  2

#ifndef COSEQ_LOG_MIN
#define COSEQ_LOG_MIN    COSEQ_LOG_INFO   /* 既定=INFO以上(DEBUG はコンパイル除去) */
#endif

/* level は COSEQ_LOG_* のいずれか。printf 互換(format, ...)。 */
typedef int (*coseq_log_cb) (int level, const char *format, ...);
void coseq_set_log_cb (coseq_log_cb cb);

/* 受信情報 */
typedef struct coseq_src {
	uint8_t        thread_idx;   /* 送り元 module idx */
	uint8_t        seq_idx;
	uint32_t       req_id;
	coseq_result_t result;
	uint8_t        client_id;    /* notify 受信時に有効 */
	struct {
		uint8_t *msg;
		size_t   size;
	} msg;
} coseq_src_t;

/* シーケンスに渡されるインタフェース(不透明) */
typedef struct coseq_if coseq_if_t;

/*
 * --- シーケンス内から使うAPI (p_if 経由。インスタンスは p_if が知っている) ---
 */
coseq_src_t *coseq_request         (coseq_if_t *p_if, uint8_t module_idx, uint8_t seq_idx,
                                    uint8_t *msg, size_t msg_size);
coseq_src_t *coseq_request_timeout (coseq_if_t *p_if, uint8_t module_idx, uint8_t seq_idx,
                                    uint8_t *msg, size_t msg_size, uint32_t timeout_msec);
/* 返信を待たずに投げる(fan-out)。req_id を返す。後で coseq_wait_reply で回収(gather)。 */
uint32_t     coseq_request_async   (coseq_if_t *p_if, uint8_t module_idx, uint8_t seq_idx,
                                    uint8_t *msg, size_t msg_size);
/* 未消費の async 返信を到着順に1件返す(req_id で突き合わせる)。無ければ NULL。 */
coseq_src_t *coseq_wait_reply      (coseq_if_t *p_if);
/* 未消費の async 返信が全て揃うまで待ち、到着順に cb を呼ぶ。回収件数を返す。 */
typedef void (*coseq_reply_cb) (coseq_src_t *r, void *user);
int          coseq_gather          (coseq_if_t *p_if, coseq_reply_cb cb, void *user);
/* 返信を要求せず送るだけ(fire-and-forget)。pending を登録せず、相手が返信しても破棄。
 * yield しない。v1 の REQUEST_OPTION__WITHOUT_REPLY 相当。 */
void         coseq_send            (coseq_if_t *p_if, uint8_t module_idx, uint8_t seq_idx,
                                    uint8_t *msg, size_t msg_size);
void         coseq_reply           (coseq_if_t *p_if, coseq_result_t result, uint8_t *msg, size_t msg_size);
void         coseq_wait_timeout    (coseq_if_t *p_if, uint32_t msec);
coseq_src_t *coseq_source          (coseq_if_t *p_if);

bool coseq_reg_notify   (coseq_if_t *p_if, uint8_t category, uint8_t *out_client_id);
bool coseq_unreg_notify (coseq_if_t *p_if, uint8_t category, uint8_t client_id);
bool coseq_notify       (coseq_if_t *p_if, uint8_t category, uint8_t *msg, size_t msg_size);

void coseq_lock   (coseq_if_t *p_if);
void coseq_unlock (coseq_if_t *p_if);

/* 実行中の自モジュール/自シーケンス/自モジュールの user データ */
uint8_t coseq_self_module (coseq_if_t *p_if);
uint8_t coseq_self_seq    (coseq_if_t *p_if);
void   *coseq_self_user   (coseq_if_t *p_if);
/* 実行中シーケンスの登録名(notify ハンドラは "recv_notify")。未設定なら "" */
const char *coseq_self_seq_name (coseq_if_t *p_if);

/*
 * --- 登録 ---
 */
typedef void (*coseq_seq_fn) (coseq_if_t *p_if);

typedef struct coseq_seq {
	coseq_seq_fn fn;
	const char  *name;   /* must be non-null */
} coseq_seq_t;

typedef struct coseq_reg {
	const char        *name;         /* must be non-null */
	uint8_t            nr_que_max;
	const coseq_seq_t *seq_array;
	uint8_t            nr_seq_max;
	coseq_seq_fn       recv_notify;  /* notify 受信ハンドラ(不要なら NULL) */
	void              *user;         /* 任意のユーザデータ(coseq_self_user で取得) */
} coseq_reg_t;

/*
 * --- インスタンス(マネージャ) ---
 *   create_coseq() でハンドルを生成し、メソッドは self を第一引数に取る。
 */
typedef struct coseq_ctx_private coseq_ctx_private_t;
typedef struct coseq_ctx_if      coseq_ctx_if_t;

struct coseq_ctx_if {
	bool         (*setup)         (coseq_ctx_if_t *self, const coseq_reg_t *tbl, uint8_t nr_tbl);
	void         (*teardown)      (coseq_ctx_if_t *self);

	/* 外部スレッド(非モジュール)からの要求 */
	coseq_src_t *(*request_sync)  (coseq_ctx_if_t *self, uint8_t module_idx, uint8_t seq_idx,
	                               uint8_t *msg, size_t msg_size);
	void         (*request_async) (coseq_ctx_if_t *self, uint8_t module_idx, uint8_t seq_idx,
	                               uint8_t *msg, size_t msg_size);

	void         (*destroy)       (coseq_ctx_if_t *self);

	coseq_ctx_private_t *impl;   /* 'private' は C++ 予約語なので impl */
};

coseq_ctx_if_t *create_coseq (void);

#ifdef __cplusplus
}
#endif

#endif /* _COSEQ_IF_H_ */
