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

#include "TuneThread.h"

#include "modules.h"


CTuneThread::CTuneThread (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
	,mState (CLOSED)
{
	SEQ_BASE_t seqs [EN_SEQ_TUNE_THREAD_NUM] = {
		{(PFN_SEQ_BASE)&CTuneThread::moduleUp, (char*)"moduleUp"},     // EN_SEQ_TUNE_THREAD_MODULE_UP
		{(PFN_SEQ_BASE)&CTuneThread::moduleDown, (char*)"moduleDown"}, // EN_SEQ_TUNE_THREAD_MODULE_DOWN
		{(PFN_SEQ_BASE)&CTuneThread::tune, (char*)"tune"},             // EN_SEQ_TUNE_THREAD_TUNE
	};
	setSeqs (seqs, EN_SEQ_TUNE_THREAD_NUM);
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


#if 0
	// ここ以下はループが回りスレッド占有するので
	// 先にリプライしときます
	pIf->reply (EN_THM_RSLT_SUCCESS);


	////////////////////////////////////////
	uint32_t freq = *(uint32_t*)(pIf->getSrcInfo()->msg.pMsg);

	if (it9175_open ()) {
		it9175_tune (freq);
	}
	////////////////////////////////////////
#endif


	CTuneThread::TUNE_PARAM_t param = *(CTuneThread::TUNE_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	if (mState != CLOSED) {
		_UTL_LOG_E ("mState != CLOSED");
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->setSectId (sectId, enAct);
		return;
	}

	int r = 0;
	int pipeC2P [2];
	r = pipe(pipeC2P);
	if (r < 0) {
		_UTL_PERROR("pipe");
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->setSectId (sectId, enAct);
		return;
	}

	int pipeC2P_err [2];
	r = pipe(pipeC2P_err);
	if (r < 0) {
		_UTL_PERROR("pipe");
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->setSectId (sectId, enAct);
		return;
	}

	pid_t pidChild = fork();
	if (pidChild == -1) {
		_UTL_PERROR("fork");
		close (pipeC2P[0]);
		close (pipeC2P[1]);
		close (pipeC2P_err[0]);
		close (pipeC2P_err[1]);
		pIf->reply (EN_THM_RSLT_ERROR);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		pIf->setSectId (sectId, enAct);
		return;

	} else if (pidChild == 0) {
		// ----------------- child -----------------
		close (pipeC2P[0]);
		dup2 (pipeC2P[1], STDOUT_FILENO);
		close (pipeC2P[1]);
		
		close (pipeC2P_err[0]);
		dup2 (pipeC2P_err[1], STDERR_FILENO);
		close (pipeC2P_err[1]);

//		char *p_com_form = (char*)"./bin/recfsusb2i --np %d - -";
		char *p_com_form = (char*)"./bin/recdvb %d - -";
		char com_str [128] = {0};
		snprintf (com_str, sizeof(com_str), p_com_form, CTsAribCommon::freqKHz2pysicalCh(param.freq));
		std::vector<std::string> com = CUtils::split (com_str, ' ');
		char *p_coms[com.size() +1] ;
		_UTL_LOG_I ("execv args");
		for (size_t i = 0; i < com.size(); ++ i) {
			p_coms [i] = (char*)com[i].c_str();
			_UTL_LOG_I ("  [%s]", p_coms [i]);
		}
		p_coms [com.size()] = NULL;
		r = execv (com[0].c_str(), p_coms);
		if (r < 0) {
			_UTL_PERROR("exec");
			pIf->reply (EN_THM_RSLT_ERROR);
			sectId = THM_SECT_ID_INIT;
			enAct = EN_THM_ACT_DONE;
			pIf->setSectId (sectId, enAct);
			return;
		}
	}

	// ----------------- parent -----------------
	_UTL_LOG_I ("child pid:[%lu]", pidChild);
	close (pipeC2P[1]);

	_UTL_LOG_I ("mState => OPENED");
	mState = OPENED;

	// ここ以下はループが回りスレッド占有するので
	// 先にリプライしときます
	pIf->reply (EN_THM_RSLT_SUCCESS);

	_UTL_LOG_I ("mState => TUNE_BEGINED");
	mState = TUNE_BEGINED;

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

	int fd = pipeC2P[0];
	int fd_err = pipeC2P_err[0];
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
				if (mState == TUNE_BEGINED) {
					_UTL_LOG_I ("first ts read _size=[%d]", _size);
					_UTL_LOG_I ("mState => TUNED");
					mState = TUNED;
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
	mState = TUNE_ENDED;

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

	close (fd);
	close (fd_err);

	r = kill (pidChild, SIGUSR1);
	if (r < 0) {
		_UTL_PERROR ("kill");
	}

	_UTL_LOG_I ("kill [%lu]", pidChild);

	int _status = 0;
	r = waitpid (pidChild, &_status, 0);
	if (r < 0) {
		_UTL_PERROR ("waitpid");
	}
	if (WIFEXITED(_status)) {
		_UTL_LOG_I ("child is exit successful.");
	} else {
		_UTL_LOG_I ("child is exit failure.");
	}

	_UTL_LOG_I ("mState => CLSOED");
	mState = CLOSED;


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}
