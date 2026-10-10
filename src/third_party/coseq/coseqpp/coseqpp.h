/*
 * coseqpp.h --- coseq C++ ラッパー: manager
 *
 * C コアがインスタンス形式(create_coseq)なので manager もインスタンス化(非シングルトン)。
 */
#ifndef _COSEQPP_HH_
#define _COSEQPP_HH_

#include <vector>
#include <memory>

#include "coseqpp_if.h"
#include "coseqpp_base.h"

namespace coseq {

class manager {
public:
	manager (void);
	~manager (void);

	bool setup    (std::vector<std::shared_ptr<module_base>> &modules);
	void teardown (void);

	// 外部スレッド(非モジュール)からの要求
	source request_sync  (uint8_t module_idx, uint8_t seq_idx, uint8_t *msg = nullptr, size_t len = 0);
	void   request_async (uint8_t module_idx, uint8_t seq_idx, uint8_t *msg = nullptr, size_t len = 0);

	// --- 内部: C コアからの dispatch 先(coseq_self_user 経由で対象モジュールへ) ---
	static void dispatch_seq    (module_base *mb, uint8_t seq_idx, coseq_if_t *p_if);
	static void dispatch_notify (module_base *mb, coseq_if_t *p_if);

private:
	coseq_ctx_if_t                          *ctx_;      // C インスタンス
	std::vector<std::shared_ptr<module_base>> hold_;    // 生存維持
	std::vector<std::vector<coseq_seq_t>>     c_seqs_;  // C 用 seq 配列(寿命維持)
	std::vector<coseq_reg_t>                  c_tbl_;   // C 用登録テーブル(寿命維持)
};

} // namespace coseq

#endif /* _COSEQPP_HH_ */
