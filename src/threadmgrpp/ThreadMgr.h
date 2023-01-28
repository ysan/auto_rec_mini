#ifndef _THREADMGR_HH_
#define _THREADMGR_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>
#include <memory>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"
#include "ThreadMgrBase.h"


namespace threadmgr {

class CThreadMgr
{
public:
	static CThreadMgr *get_instance (void);

	bool setup (CThreadMgrBase* p_threads[], int thread_max);
	bool setup (std::shared_ptr<CThreadMgrBase> threads[], int thread_max);
	bool setup (std::vector<std::shared_ptr<CThreadMgrBase>> &threads);
	void wait (void);
	void teardown (void);

	CThreadMgrExternalIf * get_external_if (void) const;


private:
	// singleton
	CThreadMgr (void);
	virtual ~CThreadMgr (void);

	bool register_threads (CThreadMgrBase *threads[], int thread_max);
	void unregister_threads (void);

	bool setup (void);


	std::vector <CThreadMgrBase*> m_threads;
	int m_thread_max;

	ST_THM_REG_TBL *mp_reg_table;
	CThreadMgrExternalIf *mp_ext_if;
};

} // namespace threadmgr

#endif
