#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ChannelManager.h"
#include "modules.h"

#include "Settings.h"


CChannelManager::CChannelManager (char *pszName, uint8_t nQueNum)
	:CThreadMgrBase (pszName, nQueNum)
{
	SEQ_BASE_t seqs [EN_SEQ_CHANNEL_MANAGER__NUM] = {
		{(PFN_SEQ_BASE)&CChannelManager::onReq_moduleUp, (char*)"onReq_moduleUp"},                                                           // EN_SEQ_CHANNEL_MANAGER__MODULE_UP
		{(PFN_SEQ_BASE)&CChannelManager::onReq_moduleDown, (char*)"onReq_moduleDown"},                                                       // EN_SEQ_CHANNEL_MANAGER__MODULE_DOWN
		{(PFN_SEQ_BASE)&CChannelManager::onReq_channelScan, (char*)"onReq_channelScan"},                                                     // EN_SEQ_CHANNEL_MANAGER__CHANNEL_SCAN
		{(PFN_SEQ_BASE)&CChannelManager::onReq_getPysicalChannelByServiceId, (char*)"onReq_getPysicalChannelByServiceId"},                   // EN_SEQ_CHANNEL_MANAGER__GET_PYSICAL_CHANNEL_BY_SERVICE_ID
		{(PFN_SEQ_BASE)&CChannelManager::onReq_getPysicalChannelByRemoteControlKeyId, (char*)"onReq_getPysicalChannelByRemoteControlKeyId"}, // EN_SEQ_CHANNEL_MANAGER__GET_PYSICAL_CHANNEL_BY_REMOTE_CONTROL_KEY_ID
		{(PFN_SEQ_BASE)&CChannelManager::onReq_getChannels, (char*)"onReq_getChannels"},                                                     // EN_SEQ_CHANNEL_MANAGER__GET_CHANNELS
		{(PFN_SEQ_BASE)&CChannelManager::onReq_getTransportStreamName, (char*)"onReq_getTransportStreamName"},                               // EN_SEQ_CHANNEL_MANAGER__GET_TRANSPORT_STREAM_NAME
		{(PFN_SEQ_BASE)&CChannelManager::onReq_getServiceName, (char*)"onReq_getServiceName"},                                               // EN_SEQ_CHANNEL_MANAGER__GET_SERVICE_NAME
		{(PFN_SEQ_BASE)&CChannelManager::onReq_dumpChannels, (char*)"onReq_dumpChannels"},                                                   // EN_SEQ_CHANNEL_MANAGER__DUMP_SCAN_RESULTS
	};
	setSeqs (seqs, EN_SEQ_CHANNEL_MANAGER__NUM);


	m_channels.clear ();

}

CChannelManager::~CChannelManager (void)
{
	m_channels.clear ();
}


void CChannelManager::onReq_moduleUp (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	switch (sectId) {
	case SECTID_ENTRY:

		loadChannels ();
		dumpChannels_simple ();


		sectId = SECTID_END;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END:
		pIf->reply (EN_THM_RSLT_SUCCESS);
		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CChannelManager::onReq_moduleDown (CThreadMgrIf *pIf)
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

void CChannelManager::onReq_channelScan (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_REQ_OPEN,
		SECTID_WAIT_OPEN,
		SECTID_CHECK_FREQ,
		SECTID_REQ_TUNE,
		SECTID_WAIT_TUNE,
		SECTID_WAIT_AFTER_TUNE,
		SECTID_REQ_GET_NETWORK_INFO,
		SECTID_WAIT_GET_NETWORK_INFO,
		SECTID_REQ_GET_SERVICE_INFOS,
		SECTID_WAIT_GET_SERVICE_INFOS,
		SECTID_REQ_TUNE_STOP,
		SECTID_WAIT_TUNE_STOP,
		SECTID_NEXT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);

	static uint16_t s_ch = UHF_PHYSICAL_CHANNEL_MIN;	
	static PSISI_NETWORK_INFO s_network_info = {0};	
	static PSISI_SERVICE_INFO s_service_infos[10];
	EN_THM_RSLT enRslt = EN_THM_RSLT_SUCCESS;
	static uint8_t s_group_id = 0xff;


	switch (sectId) {
	case SECTID_ENTRY:
		pIf->lock();

		// 先にreplyしておきます
		pIf->reply (EN_THM_RSLT_SUCCESS);

		m_channels.clear ();

		sectId = SECTID_REQ_OPEN;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_OPEN: {

		CTunerServiceIf _if (getExternalIf());
		_if.reqOpen ();

		sectId = SECTID_WAIT_OPEN;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_OPEN:
		enRslt = getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			s_group_id = *(uint8_t*)(getIf()->getSrcInfo()->msg.pMsg);
			_UTL_LOG_I ("reqOpen group_id:[0x%02x]", s_group_id);
			sectId = SECTID_CHECK_FREQ;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_E ("someone is using a tuner.");
			sectId = SECTID_END;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_CHECK_FREQ:

		if (s_ch >= UHF_PHYSICAL_CHANNEL_MIN && s_ch <= UHF_PHYSICAL_CHANNEL_MAX) {
			sectId = SECTID_REQ_TUNE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			sectId = SECTID_END;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE: {

		uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (s_ch);
		_UTL_LOG_I ("(%s) ------  pysical channel:[%d] -> freq:[%d]kHz", pIf->getSeqName(), s_ch, freq);

		CTunerServiceIf::tune_param_t param = {
			s_ch,
			s_group_id
		};

		CTunerServiceIf _if (getExternalIf());
		_if.reqTune (&param);

		sectId = SECTID_WAIT_TUNE;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_WAIT_AFTER_TUNE;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_W ("tune is failure -> skip");
			sectId = SECTID_REQ_TUNE_STOP;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_WAIT_AFTER_TUNE:
		sectId = SECTID_REQ_GET_NETWORK_INFO;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_REQ_GET_NETWORK_INFO: {

		CPsisiManagerIf _if (getExternalIf(), s_group_id);
		_if.reqGetCurrentNetworkInfo (&s_network_info);

		sectId = SECTID_WAIT_GET_NETWORK_INFO;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_GET_NETWORK_INFO:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			sectId = SECTID_REQ_GET_SERVICE_INFOS;
			enAct = EN_THM_ACT_CONTINUE;

		} else {
			_UTL_LOG_W ("network info is not found -> skip");
			sectId = SECTID_REQ_TUNE_STOP;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_GET_SERVICE_INFOS: {
		CPsisiManagerIf _if (getExternalIf(), s_group_id);
		_if.reqGetCurrentServiceInfos (s_service_infos, 10);

		sectId = SECTID_WAIT_GET_SERVICE_INFOS;
		enAct = EN_THM_ACT_WAIT;

        }
		break;

	case SECTID_WAIT_GET_SERVICE_INFOS:
		enRslt = pIf->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			int n_svc = *(int*)(pIf->getSrcInfo()->msg.pMsg);
			if (n_svc > 0) {

				CChannel::service _services [10];
				for (int i = 0; i < n_svc; ++ i) {
					_services[i].service_id = s_service_infos[i].service_id;
					_services[i].service_type = s_service_infos[i].service_type;
					_services[i].service_name = s_service_infos[i].p_service_name_char;
				}

				CChannel r;
				r.set (
					s_ch,
					s_network_info.transport_stream_id,
					s_network_info.original_network_id,
					(const char*)s_network_info.network_name_char,
					s_network_info.area_code,
					s_network_info.remote_control_key_id,
					(const char*)s_network_info.ts_name_char,
					_services,
					n_svc
				);

				if (!isDuplicateChannel (&r)) {
					r.dump();
					m_channels.insert (pair<uint16_t, CChannel>(s_ch, r));
				}

				sectId = SECTID_REQ_TUNE_STOP;
				enAct = EN_THM_ACT_CONTINUE;

			} else {
				_UTL_LOG_W ("reqGetCurrentServiceInfos  num is 0 -> skip");
				sectId = SECTID_REQ_TUNE_STOP;
				enAct = EN_THM_ACT_CONTINUE;
			}

		} else {
			_UTL_LOG_W ("service infos is not found -> skip");
			sectId = SECTID_REQ_TUNE_STOP;
			enAct = EN_THM_ACT_CONTINUE;
		}
		break;

	case SECTID_REQ_TUNE_STOP: {
		CTunerServiceIf _if (getExternalIf());
		_if.reqTuneStop (s_group_id);

		sectId = SECTID_WAIT_TUNE_STOP;
		enAct = EN_THM_ACT_WAIT;
		}
		break;

	case SECTID_WAIT_TUNE_STOP:
		// とくに結果はみません
		sectId = SECTID_NEXT;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_NEXT:

		// inc pysical ch
		++ s_ch;

		memset (&s_network_info, 0x00, sizeof (s_network_info));
		memset (&s_service_infos, 0x00, sizeof (s_service_infos));

		sectId = SECTID_CHECK_FREQ;
		enAct = EN_THM_ACT_CONTINUE;
		break;

	case SECTID_END:
		pIf->unlock();

		// reset pysical ch
		s_ch = UHF_PHYSICAL_CHANNEL_MIN;

		memset (&s_network_info, 0x00, sizeof (s_network_info));
		memset (&s_service_infos, 0x00, sizeof (s_service_infos));

		dumpChannels ();
		saveChannels ();

		_UTL_LOG_I ("channel scan end.");


		//-----------------------------//
		{
			uint32_t opt = getRequestOption ();
			opt |= REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);

			// 選局を停止しときます tune stop
			// とりあえず投げっぱなし (REQUEST_OPTION__WITHOUT_REPLY)
			CTunerServiceIf _if (getExternalIf());
			_if.reqTuneStop (s_group_id);
			_if.reqClose (s_group_id);

			opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
			setRequestOption (opt);
		}
		//-----------------------------//


		s_group_id = 0xff;

		sectId = THM_SECT_ID_INIT;
		enAct = EN_THM_ACT_DONE;
		break;

	default:
		break;
	}

	pIf->setSectId (sectId, enAct);
}

void CChannelManager::onReq_getPysicalChannelByServiceId (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CChannelManagerIf::SERVICE_ID_PARAM_t param =
			*(CChannelManagerIf::SERVICE_ID_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	uint16_t _ch = getPysicalChannelByServiceId (
						param.transport_stream_id,
						param.original_network_id,
						param.service_id
					);
	if (_ch == 0xffff) {

		_UTL_LOG_E ("getPysicalChannelByServiceId is failure.");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {

		// リプライmsgに結果を乗せます
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&_ch, sizeof(_ch));

	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CChannelManager::onReq_getPysicalChannelByRemoteControlKeyId (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CChannelManagerIf::REMOTE_CONTROL_ID_PARAM_t param =
			*(CChannelManagerIf::REMOTE_CONTROL_ID_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	uint16_t _ch = getPysicalChannelByRemoteControlKeyId (
						param.transport_stream_id,
						param.original_network_id,
						param.remote_control_key_id
					);
	if (_ch == 0xffff) {

		_UTL_LOG_E ("getPysicalChannelByRemoteControlKeyId is failure.");
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {

		// リプライmsgに結果を乗せます
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&_ch, sizeof(_ch));

	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CChannelManager::onReq_getChannels (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CChannelManagerIf::REQ_CHANNELS_PARAM_t _param =
				*(CChannelManagerIf::REQ_CHANNELS_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);
	if (!_param.p_out_channels || _param.array_max_num == 0) {
		pIf->reply (EN_THM_RSLT_ERROR);

	} else {
		int n = getChannels (_param.p_out_channels, _param.array_max_num);

		// reply msgで格納数を渡します
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)&n, sizeof(n));
	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CChannelManager::onReq_getTransportStreamName (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CChannelManagerIf::SERVICE_ID_PARAM_t _param =
				*(CChannelManagerIf::SERVICE_ID_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	const char *pname = getTransportStreamName (
							_param.transport_stream_id,
							_param.original_network_id
						);

	if (pname && strlen(pname) > 0) {
		// reply msgで nameを渡します
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)pname, strlen(pname));
	} else {
		pIf->reply (EN_THM_RSLT_ERROR);
	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CChannelManager::onReq_getServiceName (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	CChannelManagerIf::SERVICE_ID_PARAM_t _param =
				*(CChannelManagerIf::SERVICE_ID_PARAM_t*)(pIf->getSrcInfo()->msg.pMsg);

	const char *pname = getServiceName (
							_param.transport_stream_id,
							_param.original_network_id,
							_param.service_id
						);

	if (pname && strlen(pname) > 0) {
		// reply msgで nameを渡します
		pIf->reply (EN_THM_RSLT_SUCCESS, (uint8_t*)pname, strlen(pname));
	} else {
		pIf->reply (EN_THM_RSLT_ERROR);
	}


	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

void CChannelManager::onReq_dumpChannels (CThreadMgrIf *pIf)
{
	uint8_t sectId;
	EN_THM_ACT enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	sectId = pIf->getSectId();
	_UTL_LOG_D ("(%s) sectId %d\n", pIf->getSeqName(), sectId);


	dumpChannels ();


	pIf->reply (EN_THM_RSLT_SUCCESS);

	sectId = THM_SECT_ID_INIT;
	enAct = EN_THM_ACT_DONE;
	pIf->setSectId (sectId, enAct);
}

uint16_t CChannelManager::getPysicalChannelByServiceId (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		uint16_t ch = iter->first;
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (
				(p_ch->transport_stream_id == _transport_stream_id) &&
				(p_ch->original_network_id == _original_network_id)
			) {
				std::vector<CChannel::service>::const_iterator iter = p_ch->services.begin();
				for (; iter != p_ch->services.end(); ++ iter) {
					if (_service_id == iter->service_id) {
						return ch;
					}
				}
			}
		}
	}

	// not found
	return 0xffff;
}

uint16_t CChannelManager::getPysicalChannelByRemoteControlKeyId (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint8_t _remote_control_key_id
) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		uint16_t ch = iter->first;
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (
//TODO
// 暫定remote_control_key_idだけで判定
//   m_channels に同じremote_control_key_idのCChannel がある場合は
//   検索順で先のものが得られるので あまりよくないやりかたです
//   --> 引数 transport_stream_id, original_network_idで切り分けすれば問題ないはず
//   --> getPysicalChannelByServiceId を使うべき
//				(p_ch->transport_stream_id == _transport_stream_id) &&
//				(p_ch->original_network_id == _original_network_id) &&
				(p_ch->remote_control_key_id == _remote_control_key_id)
			) {
				return ch;
			}
		}
	}

	// not found
	return 0xffff;
}

bool CChannelManager::isDuplicateChannel (const CChannel* p_channel) const
{
	if (!p_channel) {
		return false;
	}

	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (*p_ch == *p_channel) {
				// duplicate
				return true;
			}
		}
	}

	return false;
}

const CChannel* CChannelManager::findChannel (uint16_t pych) const
{
	std::map<uint16_t, CChannel>::const_iterator iter = m_channels.find (pych);

	if (iter == m_channels.end()) {
		return NULL;
	}

	return &(iter->second);
}

int CChannelManager::getChannels (CChannelManagerIf::CHANNEL_t *p_out_channels, int array_max_num) const
{
	if (!p_out_channels || array_max_num == 0) {
		return 0;
	}

	int n = 0;

	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			p_out_channels->pysical_channel = p_ch->pysical_channel;
			p_out_channels->transport_stream_id = p_ch->transport_stream_id;
			p_out_channels->original_network_id = p_ch->original_network_id;
			p_out_channels->remote_control_key_id = p_ch->remote_control_key_id;

			int m = 0;
			std::vector<CChannel::service>::const_iterator iter_svc = p_ch->services.begin();
			for (; iter_svc != p_ch->services.end(); ++ iter_svc) {
				p_out_channels->service_ids[m] = iter_svc->service_id;
				++ m;
			}
			p_out_channels->service_num = m;


			++ p_out_channels;

			++ n;
			if (array_max_num <= n) {
				break;
			}
		}
	}

	return n;
}

const char* CChannelManager::getTransportStreamName (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id
) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (
				(p_ch->transport_stream_id == _transport_stream_id) &&
				(p_ch->original_network_id == _original_network_id)
			) {
				return p_ch->ts_name.c_str();
			}
		}
	}

	return NULL;
}

const char* CChannelManager::getServiceName (
	uint16_t _transport_stream_id,
	uint16_t _original_network_id,
	uint16_t _service_id
) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			if (
				(p_ch->transport_stream_id == _transport_stream_id) &&
				(p_ch->original_network_id == _original_network_id)
			) {
				std::vector<CChannel::service>::const_iterator iter_svc = p_ch->services.begin();
				for (; iter_svc != p_ch->services.end(); ++ iter_svc) {
					if (iter_svc->service_id == _service_id) {
						return  iter_svc->service_name.c_str();
					}
				}
			}
		}
	}

	return NULL;
}

void CChannelManager::dumpChannels (void) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			p_ch ->dump ();
		}
	}
}

void CChannelManager::dumpChannels_simple (void) const
{
	std::map <uint16_t, CChannel>::const_iterator iter = m_channels.begin();
	for (; iter != m_channels.end(); ++ iter) {
		CChannel const *p_ch = &(iter->second);
		if (p_ch) {
			p_ch ->dump_simple ();
		}
	}
}



//--------------------------------------------------------------------------------

template <class Archive>
void serialize (Archive &archive, CChannel::service &s)
{
	archive (cereal::make_nvp("service_id", s.service_id));
	archive (cereal::make_nvp("service_type", s.service_type));
	archive (cereal::make_nvp("service_name", s.service_name));
}

template <class Archive>
void serialize (Archive &archive, CChannel &r)
{
	archive (
		cereal::make_nvp("pysical_channel", r.pysical_channel),
		cereal::make_nvp("transport_stream_id", r.transport_stream_id),
		cereal::make_nvp("original_network_id", r.original_network_id),
		cereal::make_nvp("network_name", r.network_name),
		cereal::make_nvp("area_code", r.area_code),
		cereal::make_nvp("remote_control_key_id", r.remote_control_key_id),
		cereal::make_nvp("ts_name", r.ts_name),
		cereal::make_nvp("services", r.services)
	);
}

void CChannelManager::saveChannels (void)
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive out_archive (ss);
		out_archive (CEREAL_NVP(m_channels));
	}

	std::string *p_path = CSettings::getInstance()->getParams()->getChannelsJsonPath();
	std::ofstream ofs (p_path->c_str(), std::ios::out);
	ofs << ss.str();

	ofs.close();
	ss.clear();
}

void CChannelManager::loadChannels (void)
{
	std::string *p_path = CSettings::getInstance()->getParams()->getChannelsJsonPath();
	std::ifstream ifs (p_path->c_str(), std::ios::in);
	if (!ifs.is_open()) {
		_UTL_LOG_I ("channels.json is not found.");
		return;
	}

	std::stringstream ss;
	ss << ifs.rdbuf();

	cereal::JSONInputArchive in_archive (ss);
	in_archive (CEREAL_NVP(m_channels));

	ifs.close();
	ss.clear();
}
