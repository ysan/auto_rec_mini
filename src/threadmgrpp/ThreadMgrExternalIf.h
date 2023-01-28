#ifndef _THREADMGR_EXTERNAL_IF_HH_
#define _THREADMGR_EXTERNAL_IF_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "ThreadMgrIf.h"
#include "threadmgr_if.h"

namespace threadmgr {

// ST_THM_EXTERNAL_IF wrap
class CThreadMgrExternalIf
{
public:
	CThreadMgrExternalIf (ST_THM_EXTERNAL_IF *p_ext_if)
		: m_ext_if (*p_ext_if) {
	}

	CThreadMgrExternalIf (CThreadMgrExternalIf *p_ext_if)
		: m_ext_if (p_ext_if->m_ext_if) {
	}

	~CThreadMgrExternalIf (void) {
	}


	bool request_sync (uint8_t thread_idx, uint8_t sequence_idx) {
		return m_ext_if.pfnRequestSync (thread_idx, sequence_idx, NULL, 0);
	}

	bool request_sync (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen) {
		return m_ext_if.pfnRequestSync (thread_idx, sequence_idx, msg, msglen);
	}

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx) {
		return m_ext_if.pfnRequestAsync (thread_idx, sequence_idx, NULL, 0, NULL);
	}

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint32_t *p_out_req_id) {
		return m_ext_if.pfnRequestAsync (thread_idx, sequence_idx, NULL, 0, p_out_req_id);
	}

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen) {
		return m_ext_if.pfnRequestAsync (thread_idx, sequence_idx, msg, msglen, NULL);
	}

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen, uint32_t *p_out_req_id) {
		return m_ext_if.pfnRequestAsync (thread_idx, sequence_idx, msg, msglen, p_out_req_id);
	}

	void set_request_option (request_option::type option) {
		m_ext_if.pfnSetRequestOption (option);
	}

	request_option::type get_request_option (void) const {
		return m_ext_if.pfnGetRequestOption ();
	}

	bool create_external_cp (void) {
		return m_ext_if.pfnCreateExternalCp ();
	}

	void destroy_external_cp (void) {
		return m_ext_if.pfnDestroyExternalCp ();
	}

	CSource& receive_external (void) {
		ST_THM_SRC_INFO *info = m_ext_if.pfnReceiveExternal ();
		m_source.set(info);
		return m_source;
	}


private:
	ST_THM_EXTERNAL_IF &m_ext_if;
	CSource m_source;

};

} // namespace threadmgr

#endif
