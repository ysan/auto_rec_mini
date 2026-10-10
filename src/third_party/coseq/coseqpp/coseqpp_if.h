/*
 * coseqpp_if.h --- coseq C++ ラッパー: result / source / iface / ログ
 *
 * C API(coseq_if.h)を薄く包むだけ。クラス名は snake_case、メンバは末尾 _。
 * (CIf は 'if' が予約語のため iface とした)
 */
#ifndef _COSEQPP_IF_HH_
#define _COSEQPP_IF_HH_

#include <vector>
#include <string>
#include <functional>

#include "coseq_if.h"

namespace coseq {

// coseq_result_t wrap
enum class result : int {
	ignore           = COSEQ_RSLT_IGNORE,
	success          = COSEQ_RSLT_SUCCESS,
	error            = COSEQ_RSLT_ERROR,
	request_timeout  = COSEQ_RSLT_REQ_TIMEOUT,
	sequence_timeout = COSEQ_RSLT_SEQ_TIMEOUT,
};

// ログレベル(C の COSEQ_LOG_* に対応)
enum class log_level : int {
	debug = COSEQ_LOG_DEBUG,
	info  = COSEQ_LOG_INFO,
	warn  = COSEQ_LOG_WARN,
	error = COSEQ_LOG_ERROR,
};

// ログCB(C++): 整形済みメッセージを受け取る。coseq_set_log_cb を包む。
// 設定は setup 前に一度行う想定。fn を空にすると解除。
using log_fn = std::function<void (log_level level, const std::string &msg)>;
void set_log_cb (log_fn fn);

// gather_all() の戻り(各返信をコピー所有)
struct reply {
	uint32_t             req_id;
	result               rslt;
	std::vector<uint8_t> msg;
};

// coseq_src_t wrap
class source {
public:
	class message {
		friend class source;
	public:
		uint8_t *data   (void) const { return data_; }
		size_t   length (void) const { return length_; }
	private:
		uint8_t *data_   = nullptr;
		size_t   length_ = 0;
	};

	source (coseq_src_t *p = nullptr) { set(p); }

	void set (coseq_src_t *p) {
		src_ = p;
		if (p != nullptr) {
			message_.data_   = p->msg.msg;
			message_.length_ = p->msg.size;
		}
	}

	result   get_result       (void) const { return static_cast<result>(src_->result); }
	message &get_message      (void)       { return message_; }
	uint8_t  get_client_id    (void) const { return src_->client_id; }
	uint8_t  get_thread_idx   (void) const { return src_->thread_idx; }
	uint8_t  get_sequence_idx (void) const { return src_->seq_idx; }
	uint32_t get_request_id   (void) const { return src_->req_id; }

private:
	coseq_src_t *src_ = nullptr;
	message      message_;
};

// coseq_if_t wrap
class iface {
public:
	explicit iface (coseq_if_t *p_if) : if_(p_if) {}

	source &get_source (void) { src_.set(coseq_source(if_)); return src_; }

	source &request (uint8_t module_idx, uint8_t seq_idx) {
		src_.set(coseq_request(if_, module_idx, seq_idx, nullptr, 0));
		return src_;
	}
	source &request (uint8_t module_idx, uint8_t seq_idx, uint8_t *msg, size_t len) {
		src_.set(coseq_request(if_, module_idx, seq_idx, msg, len));
		return src_;
	}
	source &request_timeout (uint8_t module_idx, uint8_t seq_idx, uint32_t timeout_msec) {
		src_.set(coseq_request_timeout(if_, module_idx, seq_idx, nullptr, 0, timeout_msec));
		return src_;
	}
	source &request_timeout (uint8_t module_idx, uint8_t seq_idx, uint8_t *msg, size_t len, uint32_t timeout_msec) {
		src_.set(coseq_request_timeout(if_, module_idx, seq_idx, msg, len, timeout_msec));
		return src_;
	}

	// fan-out/gather: 投げっぱなし(req_id を返す) + まとめ待ち(到着順に1件ずつ)
	uint32_t request_async (uint8_t module_idx, uint8_t seq_idx, uint8_t *msg = nullptr, size_t len = 0) {
		return coseq_request_async(if_, module_idx, seq_idx, msg, len);
	}
	source &wait_reply (void) { src_.set(coseq_wait_reply(if_)); return src_; }

	// 返信不要の送信(fire-and-forget)
	void send (uint8_t module_idx, uint8_t seq_idx, uint8_t *msg = nullptr, size_t len = 0) {
		coseq_send(if_, module_idx, seq_idx, msg, len);
	}

	// 全ての未消費 async 返信が揃うまで待ち、到着順にコピーして返す
	// (型名 reply は同名メソッド reply() と衝突するため coseq:: で明示修飾)
	std::vector<coseq::reply> gather_all (void) {
		std::vector<coseq::reply> out;
		for (;;) {
			coseq_src_t *r = coseq_wait_reply(if_);
			if (r == nullptr) {
				break;
			}
			coseq::reply rp;
			rp.req_id = r->req_id;
			rp.rslt   = static_cast<result>(r->result);
			if (r->msg.msg != nullptr && r->msg.size > 0) {
				rp.msg.assign(r->msg.msg, r->msg.msg + r->msg.size);
			}
			out.push_back(std::move(rp));
		}
		return out;
	}

	void reply (result rslt) { coseq_reply(if_, static_cast<coseq_result_t>(rslt), nullptr, 0); }
	void reply (result rslt, uint8_t *msg, size_t len) { coseq_reply(if_, static_cast<coseq_result_t>(rslt), msg, len); }

	void wait_timeout (uint32_t msec) { coseq_wait_timeout(if_, msec); }

	bool reg_notify   (uint8_t category, uint8_t *out_client_id) { return coseq_reg_notify(if_, category, out_client_id); }
	bool unreg_notify (uint8_t category, uint8_t client_id)      { return coseq_unreg_notify(if_, category, client_id); }
	bool notify       (uint8_t category)                         { return coseq_notify(if_, category, nullptr, 0); }
	bool notify       (uint8_t category, uint8_t *msg, size_t len) { return coseq_notify(if_, category, msg, len); }

	void lock   (void) { coseq_lock(if_); }
	void unlock (void) { coseq_unlock(if_); }

	// 実行中シーケンスの登録名
	const char *get_sequence_name (void) const { return coseq_self_seq_name(if_); }

private:
	coseq_if_t *if_;
	source      src_;
};

} // namespace coseq

#endif /* _COSEQPP_IF_HH_ */
