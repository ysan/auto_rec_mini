/*
 * coseqpp_base.cpp --- module_base の out-of-line 定義(vtable アンカー)
 */
#include "coseqpp_base.h"

namespace coseq {

module_base::~module_base (void) {}

void module_base::on_receive_notify (iface * /*p_if*/) {
	// 既定は何もしない。notify を受信したいモジュールが override する。
}

} // namespace coseq
