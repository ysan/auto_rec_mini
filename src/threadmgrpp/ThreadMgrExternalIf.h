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
	CThreadMgrExternalIf (threadmgr_external_if_t *p_ext_if)
		: m_ext_if (*p_ext_if) {
	}

	CThreadMgrExternalIf (CThreadMgrExternalIf *p_ext_if)
		: m_ext_if (p_ext_if->m_ext_if) {
	}

	~CThreadMgrExternalIf (void) {
	}


	bool request_sync (uint8_t thread_idx, uint8_t sequence_idx) {
		return m_ext_if.pfn_request_sync (thread_idx, sequence_idx, NULL, 0);
	}

	bool request_sync (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen) {
		return m_ext_if.pfn_request_sync (thread_idx, sequence_idx, msg, msglen);
	}

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx) {
		return m_ext_if.pfn_request_async (thread_idx, sequence_idx, NULL, 0, NULL);
	}

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint32_t *p_out_req_id) {
		return m_ext_if.pfn_request_async (thread_idx, sequence_idx, NULL, 0, p_out_req_id);
	}

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen) {
		return m_ext_if.pfn_request_async (thread_idx, sequence_idx, msg, msglen, NULL);
	}

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen, uint32_t *p_out_req_id) {
		return m_ext_if.pfn_request_async (thread_idx, sequence_idx, msg, msglen, p_out_req_id);
	}

	void set_request_option (request_option::type option) {
		m_ext_if.pfn_set_request_option (option);
	}

	request_option::type get_request_option (void) const {
		return m_ext_if.pfn_get_request_option ();
	}

	bool create_external_cp (void) {
		return m_ext_if.pfn_create_external_cp ();
	}

	void destroy_external_cp (void) {
		return m_ext_if.pfn_destroy_external_cp ();
	}

	CSource& receive_external (void) {
		threadmgr_src_info_t *info = m_ext_if.pfn_receive_external ();
		m_source.set(info);
		return m_source;
	}


private:
	threadmgr_external_if_t &m_ext_if;
	CSource m_source;

};

} // namespace threadmgr

#endif
