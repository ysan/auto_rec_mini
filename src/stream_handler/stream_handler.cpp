#include "stream_handler.h"


namespace stream_handler_funcs {

static std::shared_ptr<arec::stream_handler::handler> s_self[arec::group::GROUP_MAX] = {nullptr};


bool setup_instance (coseq::manager *external_if) {
	if (!external_if) {
		return false;
	}

	for (uint8_t _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		s_self [_gr] = std::shared_ptr<arec::stream_handler::handler>(new arec::stream_handler::handler(external_if, _gr));
		arec::tuner_control::client_iface::ITsReceiveHandler *p = s_self[_gr].get();
		arec::tuner_control::client_iface _if (external_if, _gr);
		arec::client_reply r = _if.request_register_ts_receive_handler(&p);
		if (r.get_result() == coseq::result::success) {
			_UTL_LOG_I ("stream handler registered: [%p]", p);	
		} else {
			_UTL_LOG_E ("stream handler register failed.");
			return false;
		}
	}

	return true;
}

std::shared_ptr<arec::stream_handler::handler> get_instance (uint8_t group_id) {
	if (group_id >= arec::group::GROUP_MAX) {
		return nullptr;
	}
	return s_self[group_id];
}

}; // namespace stream_handler_funcs
