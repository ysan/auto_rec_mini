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

// ST_THM_SRC_INFO wrap
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
	explicit CSource (ST_THM_SRC_INFO *p_info)
		: mp_info(p_info)
	{
		m_message.m_data = p_info->msg.pMsg;
		m_message.m_length = p_info->msg.size;
	}
	virtual ~CSource (void) {}

	void set (ST_THM_SRC_INFO *p_info) {
		mp_info = p_info;
		m_message.m_data = p_info->msg.pMsg;
		m_message.m_length = p_info->msg.size;
	}

	uint8_t get_thread_idx (void) const {
		return mp_info->nThreadIdx;
	}

	uint8_t get_sequence_idx (void) const {
		return mp_info->nSeqIdx;
	}

	uint32_t get_request_id (void) const {
		return mp_info->nReqId;
	}

	result get_result (void) const {
		return static_cast<result>(mp_info->enRslt);
	}

	uint8_t get_client_id (void) const {
		return mp_info->nClientId;
	}

	message& get_message (void) {
		return m_message;
	}

private:
	ST_THM_SRC_INFO *mp_info;
	message m_message;
};

// ST_THM_IF wrap
class CThreadMgrIf 
{
public:
	explicit CThreadMgrIf (ST_THM_IF *p_if)
		: m_if (*p_if)
		, m_source (p_if->pstSrcInfo) {
	}

	virtual ~CThreadMgrIf (void) {
	}


	CSource &get_source (void) {
		m_source.set(m_if.pstSrcInfo); // for sync reply
		return m_source;
	}

	bool reply (result rslt) {
		return m_if.pfnReply (static_cast<EN_THM_RSLT>(rslt), NULL, 0);
	}

	bool reply (result rslt, uint8_t *msg, size_t msglen) {
		return m_if.pfnReply (static_cast<EN_THM_RSLT>(rslt), msg, msglen);
	}

	bool reg_notify (uint8_t category, uint8_t *pclient_id) {
		return m_if.pfnRegNotify (category, pclient_id);
	}

	bool unreg_notify (uint8_t category, uint8_t client_id) {
		return m_if.pfnUnRegNotify (category, client_id);
	}

	bool notify (uint8_t category) {
		return m_if.pfnNotify (category, NULL, 0);
	}

	bool notify (uint8_t category, uint8_t *msg, size_t size) {
		return m_if.pfnNotify (category, msg, size);
	}

	void set_section_id (section_id::type sect_id, action act) {
		m_if.pfnSetSectId (sect_id, static_cast<EN_THM_ACT>(act));
	}

	section_id::type get_section_id (void) const {
		return m_if.pfnGetSectId ();
	}

	void set_timeout (uint32_t timeout_msec) {
		m_if.pfnSetTimeout (timeout_msec);
	}

	void clear_timeout (void) {
		m_if.pfnClearTimeout ();
	}

	void enable_overwrite (void) {
		m_if.pfnEnableOverwrite ();
	}

	void disable_overwrite (void) {
		m_if.pfnDisableOverwrite ();
	}

	void lock (void) {
		m_if.pfnLock ();
	}

	void unlock (void) {
		m_if.pfnUnlock ();
	}

	uint8_t get_sequence_idx (void) const {
		return m_if.pfnGetSeqIdx ();
	}

	const char* get_sequence_name (void) const {
		return m_if.pfnGetSeqName ();
	}
	

private:
	ST_THM_IF &m_if;
	CSource m_source;
};

} // namespace threadmgr

#endif
