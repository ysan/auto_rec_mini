/*
 * coseqpp_base.h --- coseq C++ ラッパー: module_base
 *
 * ユーザはこれを継承し、コンストラクタで set_sequences() する。
 */
#ifndef _COSEQPP_BASE_HH_
#define _COSEQPP_BASE_HH_

#include <string>
#include <vector>
#include <functional>

#include "coseqpp_if.h"

namespace coseq {

class manager;

struct sequence_t {
	std::function<void (iface *)> seq;
	std::string                   name;
};

class module_base {
	friend class manager;

public:
	module_base (std::string name, uint8_t que_max)
		: name_(std::move(name)), que_max_(que_max), idx_(0) {}
	virtual ~module_base (void);

	uint8_t            get_idx  (void) const { return idx_; }
	const std::string &get_name (void) const { return name_; }

protected:
	void set_sequences (const std::vector<sequence_t> &s) { sequences_ = s; }
	// 配列 + 個数の形(v1 互換)
	void set_sequences (const sequence_t seqs[], uint8_t num) {
		sequences_.assign(seqs, seqs + num);
	}
	void reset_sequences (void) { sequences_.clear(); }

	// notify を受信したいモジュールはこれを override する
	virtual void on_receive_notify (iface *p_if);

private:
	std::string             name_;
	uint8_t                 que_max_;
	uint8_t                 idx_;
	std::vector<sequence_t> sequences_;
};

} // namespace coseq

#endif /* _COSEQPP_BASE_HH_ */
