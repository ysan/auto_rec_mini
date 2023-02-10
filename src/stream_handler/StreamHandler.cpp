#include "StreamHandler.h"


namespace stream_handler_funcs {

static std::shared_ptr<CStreamHandler> s_self[CGroup::GROUP_MAX] = {nullptr};


bool setup_instance (threadmgr::CThreadMgrExternalIf *external_if) {
	if (!external_if) {
		return false;
	}

	for (uint8_t _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		s_self [_gr] = std::shared_ptr<CStreamHandler>(new CStreamHandler(external_if, _gr));
		CTunerControlIf::ITsReceiveHandler *p = s_self[_gr].get();
		CTunerControlIf _if (external_if, _gr);
		_if.request_register_ts_receive_handler(&p);
		threadmgr::CSource& r = external_if-> receive_external();
		if (r.get_result() == threadmgr::result::success) {
			_UTL_LOG_I ("stream handler registered: [%p]", p);	
		} else {
			_UTL_LOG_E ("stream handler register failed.");
			return false;
		}
	}

	return true;
}

std::shared_ptr<CStreamHandler> get_instance (uint8_t group_id) {
	if (group_id >= CGroup::GROUP_MAX) {
		return nullptr;
	}
	return s_self[group_id];
}

}; // namespace stream_handler_funcs
