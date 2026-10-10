/*
 * coseqpp.cpp --- manager 実装 + C コアからの dispatch wrapper
 *
 * 全 seq に同一 wrapper を登録し、coseq_self_user() でそのモジュールの C++ オブジェクト
 * (module_base*)を得て std::function / on_receive_notify を呼ぶ。グローバル登録表なし。
 */
#include <cstdarg>
#include <cstdio>
#include <utility>

#include "coseqpp.h"
#include "coseqpp_base.h"

namespace coseq {

// C コアが呼ぶ wrapper (C リンケージ)
extern "C" {
	static void seq_wrapper (coseq_if_t *p_if) {
		module_base *mb = static_cast<module_base *>(coseq_self_user(p_if));
		manager::dispatch_seq(mb, coseq_self_seq(p_if), p_if);
	}
	static void notify_wrapper (coseq_if_t *p_if) {
		module_base *mb = static_cast<module_base *>(coseq_self_user(p_if));
		manager::dispatch_notify(mb, p_if);
	}
}

// --- ログ: C の coseq_set_log_cb を std::function で包む ---
static log_fn g_log_fn;

extern "C" {
	static int log_wrapper (int level, const char *fmt, ...) {
		if (!g_log_fn) {
			return 0;
		}
		char buf[256];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
		g_log_fn(static_cast<log_level>(level), std::string(buf));
		return 0;
	}
}

void set_log_cb (log_fn fn) {
	g_log_fn = std::move(fn);
	coseq_set_log_cb(g_log_fn ? &log_wrapper : nullptr);
}

void manager::dispatch_seq (module_base *mb, uint8_t seq_idx, coseq_if_t *p_if) {
	iface it(p_if);
	mb->sequences_[seq_idx].seq(&it);
}

void manager::dispatch_notify (module_base *mb, coseq_if_t *p_if) {
	iface it(p_if);
	mb->on_receive_notify(&it);
}

manager::manager (void) : ctx_(create_coseq()) {}

manager::~manager (void) {
	if (ctx_ != nullptr) {
		ctx_->destroy(ctx_);
		ctx_ = nullptr;
	}
}

bool manager::setup (std::vector<std::shared_ptr<module_base>> &modules) {
	hold_ = modules;

	const size_t n = modules.size();
	c_seqs_.assign(n, std::vector<coseq_seq_t>());
	c_tbl_.assign(n, coseq_reg_t());

	for (size_t i = 0; i < n; ++i) {
		module_base *mb = modules[i].get();
		mb->idx_ = static_cast<uint8_t>(i);

		std::vector<coseq_seq_t> &cseqs = c_seqs_[i];
		for (const sequence_t &s : mb->sequences_) {
			coseq_seq_t cs;
			cs.fn   = &seq_wrapper;
			cs.name = s.name.c_str();     // std::string の寿命は mb が保持
			cseqs.push_back(cs);
		}

		coseq_reg_t &reg = c_tbl_[i];
		reg.name        = mb->name_.c_str();
		reg.nr_que_max  = mb->que_max_;
		reg.seq_array   = cseqs.data();
		reg.nr_seq_max  = static_cast<uint8_t>(cseqs.size());
		reg.recv_notify = &notify_wrapper;
		reg.user        = mb;             // wrapper はこれで対象 C++ オブジェクトへ届く
	}

	return ctx_->setup(ctx_, c_tbl_.data(), static_cast<uint8_t>(n));
}

void manager::teardown (void) {
	ctx_->teardown(ctx_);
}

source manager::request_sync (uint8_t module_idx, uint8_t seq_idx, uint8_t *msg, size_t len) {
	return source(ctx_->request_sync(ctx_, module_idx, seq_idx, msg, len));
}

void manager::request_async (uint8_t module_idx, uint8_t seq_idx, uint8_t *msg, size_t len) {
	ctx_->request_async(ctx_, module_idx, seq_idx, msg, len);
}

} // namespace coseq
