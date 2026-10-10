#ifndef _TUNER_CONTROL_H_
#define _TUNER_CONTROL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <mutex>

#include "coseqpp.h"

#include "utils.h"
#include "group.h"
#include "tuner_control_if.h"


// notify category
#define _TUNER_NOTIFY							((uint8_t)0)

namespace arec {
namespace tuner_control {

class manager : public coseq::module_base, public arec::group
{
public:
	manager (std::string name, uint8_t que_max, uint8_t group_id);
	virtual ~manager (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);

	void on_tune (coseq::iface *iface);
	void on_tune_start (coseq::iface *iface);
	void on_tune_stop (coseq::iface *iface);

	void on_register_tuner_notify (coseq::iface *iface);
	void on_unregister_tuner_notify (coseq::iface *iface);

	void on_register_ts_receive_handler (coseq::iface *iface);
	void on_unregister_ts_receive_handler (coseq::iface *iface);

	void on_get_state (coseq::iface *iface);



	std::mutex* get_mutex_ts_receive_handlers (void) {
		return &mutex_ts_receive_handlers_;
	}

	client_iface::ITsReceiveHandler** get_ts_receive_handlers (void) {
		return reg_ts_receive_handlers_;
	}

private:
	int register_ts_receive_handler (client_iface::ITsReceiveHandler *p_handler);
	void unregister_ts_receive_handler (int id);
	void set_state (client_iface::tuner_state s);


	uint32_t freq_;

	std::mutex mutex_ts_receive_handlers_;
	client_iface::ITsReceiveHandler *reg_ts_receive_handlers_ [TS_RECEIVE_HANDLER_REGISTER_NUM_MAX];

	client_iface::tuner_state state_;
	bool is_req_stop_;
};

} // namespace tuner_control
} // namespace arec

#endif
