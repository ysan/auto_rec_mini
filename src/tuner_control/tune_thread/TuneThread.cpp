#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/wait.h>     

#include "Group.h"
#include "TuneThread.h"

#include "modules.h"


int forker_log_print (FILE *fp, const char* format, ...)
{
	char buff [128] = {0};
	va_list va;
	va_start (va, format);
	vsnprintf (buff, sizeof(buff), format, va);
	if (fp == stderr) {
		_UTL_LOG_E (buff);
	} else {
		_UTL_LOG_I (buff);
	}
	va_end (va);
	return 0;
}

CTuneThread::CTuneThread (char *pszName, uint8_t nQueNum, uint8_t groupId)
	:CThreadMgrBase (pszName, nQueNum)
	,CGroup (groupId)
	,mState (state::CLOSED)
{
	SEQ_BASE_t seqs [EN_SEQ_TUNE_THREAD_NUM] = {
		{(PFN_SEQ_BASE)&CTuneThread::moduleUp, (char*)"moduleUp"},     // EN_SEQ_TUNE_THREAD_MODULE_UP
		{(PFN_SEQ_BASE)&CTuneThread::moduleDown, (char*)"moduleDown"}, // EN_SEQ_TUNE_THREAD_MODULE_DOWN
		{(PFN_SEQ_BASE)&CTuneThread::tune, (char*)"tune"},             // EN_SEQ_TUNE_THREAD_TUNE
		{(PFN_SEQ_BASE)&CTuneThread::forceKill, (char*)"forceKill"},   // EN_SEQ_TUNE_THREAD_FORCE_KILL
	};
	setSeqs (seqs, EN_SEQ_TUNE_THREAD_NUM);

	mp_settings = CSettings::getInstance();
	m_forker.set_log_cb (forker_log_print);
}

CTuneThread::~CTuneThread (void)
{
}


void CTuneThread::moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//
// do nothing
//

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTuneThread::moduleDown (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

//
// do nothing
//

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTuneThread::tune (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CTuneThread::TUNE_PARAM_t param = *(CTuneThread::TUNE_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	if (mState != state::CLOSED) {
		_UTL_LOG_E ("mState != CLOSED");
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->setSectId (sectId, enAct);
		return;
	}

	// child process command
	std::vector<std::string> *p_tuner_hal_allocates = mp_settings->getParams()->getTunerHalAllocates();
	if (p_tuner_hal_allocates->size() <= getGroupId()) {
		_UTL_LOG_E ("not allocated tuner hal command... (settings.json ->tuner_hal_allocates)");
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->setSectId (sectId, enAct);
		return;
	}
	std::string com_form = (*p_tuner_hal_allocates) [getGroupId()];
	char com_str [128] = {0};
	snprintf (com_str, sizeof(com_str), com_form.c_str(), CTsAribCommon::freqKHz2pysicalCh(param.freq));
	std::string command = com_str;

	if (!m_forker.create_pipes()) {
		_UTL_LOG_E("create_pipes failure.");
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->setSectId (sectId, enAct);
		m_forker.destroy_pipes();
		return;
	}

	if (!m_forker.do_fork(command)) {
		_UTL_LOG_E("do_dork failure.");
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->setSectId (sectId, enAct);
		m_forker.destroy_pipes();
		return;
	}

	_UTL_LOG_I ("mState => OPENED");
	mState = state::OPENED;

	// ここ以下はループが回りスレッド占有するので
	// 先にリプライしときます
	pIf->reply (EN_THM_RSLT_SUCCESS);

	_UTL_LOG_I ("mState => TUNE_BEGINED");
	mState = state::TUNE_BEGINED;

	{ //---------------------------
		_UTL_LOG_I ("onPreTsReceive");
		std::lock_guard<std::mutex> lock (*param.pMutexTsReceiveHandlers);
		CTunerControlIf::ITsReceiveHandler **p_handlers = param.pTsReceiveHandlers;
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				(p_handlers[i])->onPreTsReceive ();
			}
		}
	} //---------------------------

	int r = 0;
//	int fd = pipeC2P[0];
//	int fd_err = pipeC2P_err[0];
	int fd = m_forker.get_child_stdout_fd();
	int fd_err = m_forker.get_child_stderr_fd();
	uint8_t buff [1024] = {0};
	uint64_t _total_size = 0;

	fd_set _fds;
	struct timeval _timeout;

	while (1) {
		FD_ZERO (&_fds);
		FD_SET (fd, &_fds);
		FD_SET (fd_err, &_fds);
		_timeout.tv_sec = 1;
		_timeout.tv_usec = 0;
		r = select (fd > fd_err ? fd + 1 : fd_err + 1, &_fds, NULL, NULL, &_timeout);
		if (r < 0) {
			_UTL_PERROR ("select");
			continue;

		} else if (r == 0) {
			// timeout
			// check tuner stop request
			if (*(param.pIsReqStop)) {
				_UTL_LOG_I ("req stoped. (timeout check)");
				break;
			}
		}

		// read stdout
		if (FD_ISSET (fd, &_fds)) {
			int _size = read (fd, buff, sizeof(buff));
			if (_size < 0) {
				_UTL_PERROR ("read");
				break;

			} else if (_size == 0) {
				// file end
				_UTL_LOG_I ("ts read end");
				break;

			} else {
				if (mState == state::TUNE_BEGINED) {
					_UTL_LOG_I ("first ts read _size=[%d]", _size);
					_UTL_LOG_I ("mState => TUNED");
					mState = state::TUNED;
				}

				{ //---------------------------
					std::lock_guard<std::mutex> lock (*param.pMutexTsReceiveHandlers);
					CTunerControlIf::ITsReceiveHandler **p_handlers = param.pTsReceiveHandlers;
					for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
						if (p_handlers[i]) {
							(p_handlers[i])->onTsReceived (buff, _size);
						}
					}
				} //---------------------------

				_total_size += _size;

				// check tuner stop request
				if (*(param.pIsReqStop)) {
					_UTL_LOG_I ("req stoped.");
					break;
				}
			}
		}

		// read stderr
		if (FD_ISSET (fd_err, &_fds)) {
			memset (buff, 0x00, sizeof(buff));
			int _size = read (fd_err, buff, sizeof(buff));
			if (_size < 0) {
				_UTL_PERROR ("read");
				break;

			} else if (_size == 0) {
				// file end
				_UTL_LOG_I ("ts read end");
				break;

			} else {
				_UTL_LOG_I ("%s", buff);

				// check tuner stop request
				if (*(param.pIsReqStop)) {
					_UTL_LOG_I ("req stoped.");
					break;
				}
			}
		}
	}
	_UTL_LOG_I ("mState => TUNE_ENDED");
	mState = state::TUNE_ENDED;

	_UTL_LOG_I ("ts read total_size=[%llu]", _total_size);

	{ //---------------------------
		_UTL_LOG_I ("onPostTsReceive");
		std::lock_guard<std::mutex> lock (*param.pMutexTsReceiveHandlers);
		CTunerControlIf::ITsReceiveHandler **p_handlers = param.pTsReceiveHandlers;
		for (int i = 0; i < TS_RECEIVE_HANDLER_REGISTER_NUM_MAX; ++ i) {
			if (p_handlers[i]) {
				(p_handlers[i])->onPostTsReceive ();
			}
		}
	} //---------------------------

	m_forker.wait_child(SIGUSR1);
	m_forker.destroy_pipes();

	_UTL_LOG_I ("mState => CLSOED");
	mState = state::CLOSED;


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CTuneThread::forceKill (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	m_forker.wait_child(SIGUSR1);
	m_forker.destroy_pipes();

	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}
