#ifndef _CLIENT_IF_H_
#define _CLIENT_IF_H_

#include "coseqpp.h"

namespace arec {

//
// 外部スレッド / ブロッキングなコマンド処理から各モジュールへ要求する
// クライアント基底 (v1 の threadmgr::CThreadMgrExternalIf 相当を coseq 用に置換)。
//
// v1 との対応:
//   - 返信を使う要求 : 旧 request_async(...) + ext->receive_external() の2段を
//                      request_sync(...) 1本に畳む。
//   - 返信不要の送信 : request_async(...) のまま (fire-and-forget)。
//
// request_sync() は client_reply を返す。これは
//   - bool として使える (成否。旧 request_sync が bool を返していた箇所は無改修)
//   - get_result()/get_message() で返信内容を取れる (旧 receive_external() の代替)
// の両対応ラッパ。
//
// 注意: 返信は coseq の thread-local static バッファを指す。同一スレッドで
//       次の request_sync を呼ぶまでの間に読むこと (http/コマンドは逐次なので安全)。
//
class client_reply
{
public:
	// 返信あり(request_sync)
	client_reply (const coseq::source &src) : src_ (src), has_src_ (true) {}
	// 返信なし(request_async の fire-and-forget)。ok は「要求を投げられたか」。
	client_reply (bool ok) : ok_ (ok), has_src_ (false) {}

	// bool 文脈: 成功なら true (非 explicit: `return request_sync/async(...)` と `if(...)` 両対応)
	operator bool (void) const {
		return has_src_ ? (src_.get_result() == coseq::result::success) : ok_;
	}

	coseq::result get_result (void) const {
		return has_src_ ? src_.get_result() : (ok_ ? coseq::result::success : coseq::result::error);
	}
	// get_message() は返信あり(request_sync 経由)のときのみ有効。
	coseq::source::message &get_message (void) { return src_.get_message(); }
	coseq::source          &get_source  (void) { return src_; }

private:
	coseq::source src_;
	bool          ok_      = false;
	bool          has_src_ = false;
};


class client_iface_base
{
public:
	explicit client_iface_base (coseq::manager *p_mgr)
		: mgr_ (p_mgr) {
	}
	client_iface_base (client_iface_base *p_base)
		: mgr_ (p_base->mgr_) {
	}
	virtual ~client_iface_base (void) = default;

protected:
	// 返信を使う同期要求 (ブロッキング)。bool にも返信取得にも使える。
	client_reply request_sync (uint8_t module_idx, uint8_t sequence_idx, uint8_t *msg = nullptr, size_t msglen = 0) {
		return client_reply (mgr_->request_sync (module_idx, sequence_idx, msg, msglen));
	}
	// 返信不要の送信 (fire-and-forget)。
	bool request_async (uint8_t module_idx, uint8_t sequence_idx, uint8_t *msg = nullptr, size_t msglen = 0) {
		mgr_->request_async (module_idx, sequence_idx, msg, msglen);
		return true;
	}

	coseq::manager *mgr_;
};

} // namespace arec

#endif
