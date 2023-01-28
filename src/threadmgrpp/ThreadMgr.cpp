#include "ThreadMgr.h"
#include "ThreadMgrExternalIf.h"


namespace threadmgr {

/*
 * Constant define
 */
// ここは threadmgr.c の宣言と同じ値に
#define THREAD_IDX_MAX					(0x20) // 32  uint8の為 255以下の値で THREAD_IDX_BLANKより小さい値
#define SEQ_IDX_MAX						(0x40) // 64  uint8の為 255以下の値で SEQ_IDX_BLANKより小さい値
#define QUE_WORKER_MIN					(5)
#define QUE_WORKER_MAX					(100)


static void dispatcher (
	EN_THM_DISPATCH_TYPE type,
	uint8_t thread_idx,
	uint8_t seq_idx,
	ST_THM_IF *p_if
);


CThreadMgrBase *gp_threads [THREAD_IDX_MAX];


CThreadMgr::CThreadMgr (void)
	:m_thread_max (0)
	,mp_reg_table (NULL)
	,mp_ext_if (NULL)
{
	m_threads.clear();
}

CThreadMgr:: ~CThreadMgr (void)
{
}


CThreadMgr* CThreadMgr::get_instance (void)
{
	static CThreadMgr singleton;
	return &singleton;
}

bool CThreadMgr::register_threads (CThreadMgrBase *p_threads[], int thread_max)
{
	if ((!p_threads) || (thread_max == 0)) {
		THM_LOG_E ("invalid argument.\n");
		return false;
	}

	int idx = 0;
	while (idx < thread_max) {
		if (idx == THREAD_IDX_MAX) {
			// over flow
			THM_LOG_E ("thread idx is over. (inner thread table)\n");
			return false;
		}

		if ((p_threads [idx]->m_que_max < QUE_WORKER_MIN) && (p_threads [idx]->m_que_max > QUE_WORKER_MAX)) {
			THM_LOG_E ("que num is invalid. (inner thread table)\n");
			return false;
		}

		if ((p_threads [idx]->m_sequence_max <= 0) && (p_threads [idx]->m_sequence_max > SEQ_IDX_MAX)) {
			THM_LOG_E ("seq idx is invalid. (inner thread table)\n");
			return false;
		}

		p_threads[idx]->set_idx(idx);
		gp_threads [idx] = p_threads [idx];
		++ idx;
	}

	return true;
}

void CThreadMgr::unregister_threads (void)
{
	int i = 0;
	while (i < THREAD_IDX_MAX) {
		gp_threads [i] = NULL;
		++ i;
	}
}

bool CThreadMgr::setup (void)
{
	mp_reg_table = reinterpret_cast<ST_THM_REG_TBL*>(malloc (sizeof(ST_THM_REG_TBL) * m_thread_max));
	if (!mp_reg_table) {
		THM_PERROR ("malloc");
		return false;
	}

	for (int i = 0; i < m_thread_max; ++ i) {
		ST_THM_REG_TBL *p = mp_reg_table + i;

		p->pszName = gp_threads[i]->m_name;
		const_cast <PCB_CREATE&> (p->pcbCreate) = NULL;				// not use
		const_cast <PCB_DESTROY&> (p->pcbDestroy) = NULL;			// not use
		p->nQueNum = gp_threads[i]->m_que_max;
		p->pstSeqArray = NULL;										// set after
		p->nSeqNum = gp_threads[i]->m_sequence_max;
		const_cast <PCB_RECV_NOTIFY&> (p->pcbRecvNotify) = NULL;	// not use


		// set seq name
		ST_THM_SEQ *p_thm_seq = reinterpret_cast<ST_THM_SEQ*>(malloc (sizeof(ST_THM_SEQ) * gp_threads[i]->m_sequence_max));
		if (!p_thm_seq) {
			THM_PERROR ("malloc");
			return false;
		}
		for (int j = 0; j < gp_threads[i]->m_sequence_max; ++ j) {
			ST_THM_SEQ *p = p_thm_seq + j;
			const_cast <PCB_THM_SEQ&> (p->pcbSeq) = NULL;
			p->pszName = ((gp_threads[i]->mp_sequences) + j)->name.c_str();
		}
		p->pstSeqArray = p_thm_seq;
	}


	ST_THM_EXTERNAL_IF* p_ext_if = setupThreadMgr (mp_reg_table, (uint32_t)m_thread_max);
	if (!p_ext_if) {
		THM_LOG_E ("setupThreadMgr() is failure.\n");
		return false;
	}

	mp_ext_if = new CThreadMgrExternalIf (p_ext_if);
	if (!mp_ext_if) {
		THM_LOG_E ("mp_ext_if is null.\n");
		return false;
	}

	// threadMgr_external_if
	for (int i = 0; i < m_thread_max; ++ i) {
		gp_threads[i]->set_external_if (mp_ext_if);
	}

	return true;
}

bool CThreadMgr::setup (CThreadMgrBase* p_threads[], int thread_max)
{
	if (!register_threads (p_threads, thread_max)) {
		THM_LOG_E ("register_threads() is failure.\n");
		return false;
	}

	m_thread_max = thread_max;

	setupDispatcher (dispatcher);

	return setup();
}

bool CThreadMgr::setup (std::shared_ptr<CThreadMgrBase> threads[], int thread_max)
{
	CThreadMgrBase *p_bases [thread_max];
	for (int i = 0; i < thread_max; ++ i) {
		p_bases[i] = threads[i].get();
	}

	if (!register_threads (p_bases, thread_max)) {
		THM_LOG_E ("register_threads() is failure.\n");
		return false;
	}

	m_thread_max = thread_max;

	setupDispatcher (dispatcher);

	return setup();
}

bool CThreadMgr::setup (std::vector<std::shared_ptr<CThreadMgrBase>> &threads)
{
	int _size = threads.size();
	CThreadMgrBase *p_bases [_size];
	int idx = 0;
	for (const auto &th : threads) {
		p_bases[idx] = th.get();
		++ idx ;
	}

	if (!setup (p_bases, _size)) {
		THM_LOG_E ("setup() is failure.\n");
		return false;
	}

	return true;
}

void CThreadMgr::wait (void)
{
	waitThreadMgr ();
}

void CThreadMgr::teardown (void)
{
	if (mp_ext_if) {
		delete mp_ext_if;
		mp_ext_if = NULL;
	}

	unregister_threads ();

	if (mp_reg_table) {
		for (int i = 0; i < m_thread_max; ++ i) {
			ST_THM_REG_TBL *p = mp_reg_table + i;
			ST_THM_SEQ* s = const_cast <ST_THM_SEQ*> (p->pstSeqArray);
			free (s);
		}

		free (mp_reg_table);
		mp_reg_table = NULL;
	}

	m_thread_max = 0;

	teardownThreadMgr ();
}

CThreadMgrExternalIf * CThreadMgr::get_external_if (void) const
{
	return mp_ext_if;
}

static void dispatcher (
	EN_THM_DISPATCH_TYPE type,
	uint8_t thread_idx,
	uint8_t seq_idx,
	ST_THM_IF *p_if
) {
	CThreadMgrBase *base = gp_threads [thread_idx];
	if (!base) {
		THM_LOG_E ("gp_threads [%d] is null.", thread_idx);
		return ;
	}

	///////////////////////////////

	base->exec (type, seq_idx, p_if);

	///////////////////////////////
}

} // namespace threadmgr
