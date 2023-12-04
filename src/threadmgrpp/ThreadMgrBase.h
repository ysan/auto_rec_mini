#ifndef _THREADMGR_BASE_HH_
#define _THREADMGR_BASE_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <vector>
#include <functional>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"


namespace threadmgr {

class CThreadMgr;
class CThreadMgrBase;
//typedef void (CThreadMgrBase:: *PFN_SEQ_BASE) (CThreadMgrIf *pIf);

struct sequence_t {
//	PFN_SEQ_BASE pfnSeqBase;
	std::function<void (CThreadMgrIf *p_if)> seq;
	std::string name;
};


class CThreadMgrBase
{
public:
	friend class CThreadMgr;


	CThreadMgrBase (const char *name, uint8_t que_max);
	CThreadMgrBase (std::string name, uint8_t que_max);
	virtual ~CThreadMgrBase (void);


	void exec (EN_THM_DISPATCH_TYPE type, uint8_t sequence_idx, threadmgr_if_t *p_if);

	CThreadMgrExternalIf *get_external_if (void) const;
	CThreadMgrIf *get_if (void) const;

	void set_idx (uint8_t idx);
	uint8_t get_idx (void) const;
	const char* get_name (void) const;


protected:
	void set_sequences (const sequence_t sequences [], uint8_t num);
	void set_sequences (const std::vector<sequence_t> &sequences);
	void reset_sequences (void);

	virtual void on_create (void);
	virtual void on_destroy (void);
	virtual void on_receive_notify (CThreadMgrIf *p_if);


	bool request_sync (uint8_t thread_idx, uint8_t sequence_idx);
	bool request_sync (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen);

	bool request_async (uint8_t thread_idx, uint8_t sequence_idx);
	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint32_t *out_req_id);
	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen);
	bool request_async (uint8_t thread_idx, uint8_t sequence_idx, uint8_t *msg, size_t msglen, uint32_t *out_req_id);

	void set_request_option (request_option::type option);
	request_option::type get_request_option (void) const;


private:
	void set_external_if (CThreadMgrExternalIf *p_ext_if);
	void set_if (CThreadMgrIf *p_if);


	const sequence_t *mp_sequences ;

	char m_name [16];
	uint8_t m_que_max;

	std::vector<sequence_t> m_sequences;
	uint8_t m_sequence_max;
	
	CThreadMgrExternalIf *mp_ext_if;
	CThreadMgrIf *mp_thm_if;

	uint8_t m_idx;
};

} // namespace ThreadManager

#endif
