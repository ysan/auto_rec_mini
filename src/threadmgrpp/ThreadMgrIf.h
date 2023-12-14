#ifndef _THREADMGR_IF_HH_
#define _THREADMGR_IF_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "threadmgr_if.h"


namespace threadmgr {

namespace section_id {
	using type = uint8_t;
	// THM_SECT_ID_INIT wrap
	constexpr uint8_t init = THM_SECT_ID_INIT;
}

// REQUEST_OPTION__xxx wrap
namespace request_option {
	using type = uint32_t;
	constexpr uint32_t without_reply     = REQUEST_OPTION__WITHOUT_REPLY;
	constexpr uint32_t with_timeout_msec = REQUEST_OPTION__WITH_TIMEOUT_MSEC;
	constexpr uint32_t with_timeout_min  = REQUEST_OPTION__WITH_TIMEOUT_MIN;
}

// EN_THM_RSLT wrap
enum class result : int {
	ignore           = EN_THM_RSLT_IGNORE,
	success          = EN_THM_RSLT_SUCCESS,
	error            = EN_THM_RSLT_ERROR,
	request_timeout  = EN_THM_RSLT_REQ_TIMEOUT,
	sequence_timeout = EN_THM_RSLT_SEQ_TIMEOUT,
	max              = EN_THM_RSLT_MAX,
};

// EN_THM_ACT wrap
enum class action : int {
	init      = EN_THM_ACT_INIT,
	continue_ = EN_THM_ACT_CONTINUE,
	wait      = EN_THM_ACT_WAIT,
	done      = EN_THM_ACT_DONE,
	max       = EN_THM_ACT_MAX,
};

// threadmgr_src_info_t wrap
class CSource {
public:
	struct message {
	friend class CSource;
	message(void) = default;
	virtual ~message(void) = default;
	public:
		uint8_t *data (void) const {
			return m_data;
		}

		size_t length (void) const {
			return m_length;
		}
	private:
		uint8_t *m_data;
		size_t m_length;
	};

	CSource (void) {}
	explicit CSource (threadmgr_src_info_t *p_info)
		: mp_info(p_info)
	{
		m_message.m_data = p_info->msg.msg;
		m_message.m_length = p_info->msg.size;
	}
	virtual ~CSource (void) {}

	void set (threadmgr_src_info_t *p_info) {
		mp_info = p_info;
		m_message.m_data = p_info->msg.msg;
		m_message.m_length = p_info->msg.size;
	}

	uint8_t get_thread_idx (void) const {
		return mp_info->thread_idx;
	}

	uint8_t get_sequence_idx (void) const {
		return mp_info->seq_idx;
	}

	uint32_t get_request_id (void) const {
		return mp_info->req_id;
	}

	result get_result (void) const {
		return static_cast<result>(mp_info->result);
	}

	uint8_t get_client_id (void) const {
		return mp_info->client_id;
	}

	message& get_message (void) {
		return m_message;
	}

private:
	threadmgr_src_info_t *mp_info;
	message m_message;
};

// threadmgr_if_t wrap
class CThreadMgrIf 
{
public:
	explicit CThreadMgrIf (threadmgr_if_t *p_if)
		: m_if (*p_if)
		, m_source (p_if->src_info) {
	}

	virtual ~CThreadMgrIf (void) {
	}


	CSource &get_source (void) {
		m_source.set(m_if.src_info); // for sync reply
		return m_source;
	}

	bool reply (result rslt) {
		return m_if.reply (static_cast<EN_THM_RSLT>(rslt), NULL, 0);
	}

	bool reply (result rslt, uint8_t *msg, size_t msglen) {
		return m_if.reply (static_cast<EN_THM_RSLT>(rslt), msg, msglen);
	}

	bool reg_notify (uint8_t category, uint8_t *pclient_id) {
		return m_if.reg_notify (category, pclient_id);
	}

	bool unreg_notify (uint8_t category, uint8_t client_id) {
		return m_if.unreg_notify (category, client_id);
	}

	bool notify (uint8_t category) {
		return m_if.notify (category, NULL, 0);
	}

	bool notify (uint8_t category, uint8_t *msg, size_t size) {
		return m_if.notify (category, msg, size);
	}

	void set_section_id (section_id::type sect_id, action act) {
		m_if.set_sectid (sect_id, static_cast<EN_THM_ACT>(act));
	}

	section_id::type get_section_id (void) const {
		return m_if.get_sectid ();
	}

	void set_timeout (uint32_t timeout_msec) {
		m_if.set_timeout (timeout_msec);
	}

	void clear_timeout (void) {
		m_if.clear_timeout ();
	}

	void enable_overwrite (void) {
		m_if.enable_overwrite ();
	}

	void disable_overwrite (void) {
		m_if.disable_overwrite ();
	}

	void lock (void) {
		m_if.lock ();
	}

	void unlock (void) {
		m_if.unlock ();
	}

	uint8_t get_sequence_idx (void) const {
		return m_if.get_seq_idx ();
	}

	const char* get_sequence_name (void) const {
		return m_if.get_seq_name ();
	}
	

private:
	threadmgr_if_t &m_if;
	CSource m_source;
};

} // namespace threadmgr

#endif
