#include "ThreadMgrBase.h"
#include "ThreadMgrIf.h"


namespace threadmgr {

CThreadMgrBase::CThreadMgrBase (const char *name, uint8_t que_max)
	:mp_sequences (NULL)
	,m_que_max (0)
	,m_sequence_max (0)
	,mp_ext_if (NULL)
	,mp_thm_if (NULL)
	,m_idx (-1)
{
	if (name && (strlen(name) > 0)) {
		memset (m_name, 0x00, sizeof(m_name));
		strncpy (m_name, name, sizeof(m_name) -1);
	} else {
		memset (m_name, 0x00, sizeof(m_name));
		strncpy (m_name, "-----", sizeof(m_name) -1);
	}

	m_que_max = que_max;

	m_sequences.clear();
}

CThreadMgrBase::CThreadMgrBase (std::string name, uint8_t que_max)
	: CThreadMgrBase (name.c_str(), que_max)
{
}

CThreadMgrBase::~CThreadMgrBase (void)
{
}


void CThreadMgrBase:: exec (EN_THM_DISPATCH_TYPE type, uint8_t sequence_idx, threadmgr_if_t *p_if)
{
	switch (type) {
	case EN_THM_DISPATCH_TYPE_CREATE:

		on_create ();

		break;

	case EN_THM_DISPATCH_TYPE_DESTROY:

		on_destroy ();

		break;

	case EN_THM_DISPATCH_TYPE_REQ_REPLY:
		{
		CThreadMgrIf thm_if (p_if);
		set_if (&thm_if);

//		(void) (this->*((mpSeqsBase + sequence_idx)->pfnSeqBase)) (&thmIf);
		(mp_sequences + sequence_idx)->seq (&thm_if);

		set_if (NULL);
		break;
		}

	case EN_THM_DISPATCH_TYPE_NOTIFY:
		{
		CThreadMgrIf thm_if (p_if);
		set_if (&thm_if);

		on_receive_notify (&thm_if);

		set_if (NULL);
		break;
		}

	default:
		THM_LOG_E ("BUG: type is unknown. (CThreadMgrBase::exec)");
		break;
	}
}

void CThreadMgrBase::set_idx (uint8_t idx)
{
	m_idx = idx;
}

uint8_t CThreadMgrBase::get_idx (void) const
{
	return m_idx;
}

const char* CThreadMgrBase::get_name (void) const
{
	return m_name;
}

void CThreadMgrBase::set_sequences (const sequence_t sequences [], uint8_t sequences_max)
{
	if (sequences && sequences_max > 0) {
		for (int i = 0; i < sequences_max; ++ i) {
			m_sequences.push_back(sequences[i]);
		}
		mp_sequences = &m_sequences[0];
		m_sequence_max = sequences_max;
	}
}

void CThreadMgrBase::set_sequences (const std::vector<sequence_t> &sequences)
{
	if (sequences.size() > 0) {
		m_sequences = sequences;
		mp_sequences = &m_sequences[0];
		m_sequence_max = sequences.size();
	}
}

void CThreadMgrBase::reset_sequences (void)
{
	m_sequences.clear();
	mp_sequences = NULL;
	m_sequence_max = 0;
}

void CThreadMgrBase::on_create (void)
{
}

void CThreadMgrBase::on_destroy (void)
{
}

void CThreadMgrBase::on_receive_notify (CThreadMgrIf *p_if)
{
}


bool CThreadMgrBase::request_sync(uint8_t thread_idx, uint8_t sequence_idx)
{
	if (!mp_ext_if) {
		THM_LOG_E ("BUG: mp_ext_if is null. (CThreadMgrBase)");
		return false;
	}

	return mp_ext_if->request_sync (thread_idx, sequence_idx);
}

bool CThreadMgrBase::request_sync (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen)
{
	if (!mp_ext_if) {
		THM_LOG_E ("BUG: mp_ext_if is null. (CThreadMgrBase)");
		return false;
	}

	return mp_ext_if->request_sync (thread_idx, sequence_idx, msg, msglen);
}

bool CThreadMgrBase::request_async (uint8_t thread_idx, uint8_t sequence_idx)
{
	if (!mp_ext_if) {
		THM_LOG_E ("BUG: mp_ext_if is null. (CThreadMgrBase)");
		return false;
	}

	return mp_ext_if->request_async (thread_idx, sequence_idx);
}

bool CThreadMgrBase::request_async (uint8_t thread_idx, uint8_t sequence_idx, uint32_t *p_out_req_id)
{
	if (!mp_ext_if) {
		THM_LOG_E ("BUG: mp_ext_if is null. (CThreadMgrBase)");
		return false;
	}

	return mp_ext_if->request_async (thread_idx, sequence_idx, p_out_req_id);
}

bool CThreadMgrBase::request_async (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen)
{
	if (!mp_ext_if) {
		THM_LOG_E ("BUG: mp_ext_if is null. (CThreadMgrBase)");
		return false;
	}

	return mp_ext_if->request_async (thread_idx, sequence_idx, msg, msglen);
}

bool CThreadMgrBase::request_async (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen, uint32_t *p_out_req_id)
{
	if (!mp_ext_if) {
		THM_LOG_E ("BUG: mp_ext_if is null. (CThreadMgrBase)");
		return false;
	}

	return mp_ext_if->request_async (thread_idx, sequence_idx, msg, msglen, p_out_req_id);
}

void CThreadMgrBase::set_request_option (request_option::type option)
{
	if (!mp_ext_if) {
		THM_LOG_E ("BUG: mp_ext_if is null. (CThreadMgrBase)");
		return ;
	}

	mp_ext_if->set_request_option (option);
}

request_option::type CThreadMgrBase::get_request_option (void) const
{
	if (!mp_ext_if) {
		THM_LOG_E ("BUG: mp_ext_if is null. (CThreadMgrBase)");
		return 0;
	}

	return mp_ext_if->get_request_option ();
}

CThreadMgrExternalIf * CThreadMgrBase::get_external_if (void) const
{
	return mp_ext_if;
}

void CThreadMgrBase::set_external_if (CThreadMgrExternalIf *p_ext_if)
{
	mp_ext_if = p_ext_if;
}

CThreadMgrIf * CThreadMgrBase::get_if (void) const
{
	return mp_thm_if;
}

void CThreadMgrBase::set_if (CThreadMgrIf *p_if)
{
	mp_thm_if = p_if;
}

} // namespace threadmgr
