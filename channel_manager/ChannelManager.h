#ifndef _CHANNEL_MANAGER_H_
#define _CHANNEL_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"

#include "ThreadMgrBase.h"
#include "ThreadMgr.h"

#include "Utils.h"
#include "TsAribCommon.h"
#include "ChannelManagerIf.h"

#include "TunerControlIf.h"
#include "PsisiManagerIf.h"

#include "cereal/cereal.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"


using namespace ThreadManager;

class CChannel {
public:
	CChannel (void) {
		clear ();
	}
	virtual ~CChannel (void) {
		clear ();
	}


	bool operator == (const CChannel &obj) const {
		if (
			this->transport_stream_id == obj.transport_stream_id &&
			this->original_network_id == obj.original_network_id
		) {
			return true;
		} else {
			return false;
		}
	}

	bool operator != (const CChannel &obj) const {
		if (
			this->transport_stream_id != obj.transport_stream_id ||
			this->original_network_id != obj.original_network_id
		) {
			return true;
		} else {
			return false;
		}
	}

	uint16_t pysical_channel;

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	std::string network_name;

	uint16_t area_code;
	uint8_t remote_control_key_id;
	std::string ts_name;

	struct service {
		uint16_t service_id;
		uint8_t service_type;
		std::string service_name;

		void clear (void) {
			service_id = 0;
			service_type = 0;
			service_name.clear();
			service_name.shrink_to_fit();
		}
	};
	std::vector <service> services;

	void set (
		uint16_t _pysical_channel,
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		const char *psz_network_name,
		uint16_t _area_code,
		uint8_t _remote_control_key_id,
		const char *psz_ts_name,
		struct service _services[],
		int _services_num
	) {
		this->pysical_channel = _pysical_channel;
		this->transport_stream_id = _transport_stream_id;
		this->original_network_id = _original_network_id;
		if (psz_network_name) {
			this->network_name = psz_network_name ;
		}
		this->area_code = _area_code;
		this->remote_control_key_id = _remote_control_key_id;
		if (psz_ts_name) {
			this->ts_name = psz_ts_name ;
		}
		if (_services && _services_num > 0) {
			for (int i = 0; i < _services_num; ++ i) {
				services.push_back(_services[i]);
			}
		}
	}

	void clear (void) {
		pysical_channel = 0;
		transport_stream_id = 0;
		original_network_id = 0;
		network_name.clear();
		network_name.shrink_to_fit();
		area_code = 0;
		remote_control_key_id = 0;
		ts_name.clear();
		ts_name.shrink_to_fit();

		std::vector<service>::iterator iter = services.begin();
		for (; iter != services.end(); ++ iter) {
			iter->clear();
		}
		services.clear();
		services.shrink_to_fit();
	}

	void dump (void) const {
		uint32_t freq = CTsAribCommon::pysicalCh2freqKHz (pysical_channel);
		_UTL_LOG_I ("-------------  pysical channel:[%d] ([%d]kHz) -------------", pysical_channel, freq);
		_UTL_LOG_I (
			"pych:[%d] tsid:[0x%04x] org_nid:[0x%04x]",
			pysical_channel,
			transport_stream_id,
			original_network_id
		);
		_UTL_LOG_I ("network_name:[%s]", network_name.c_str());
		_UTL_LOG_I (
			"area_code:[0x%04x] remote_control_key_id:[0x%02x] ts_name:[%s]",
			area_code,
			remote_control_key_id,
			ts_name.c_str()
		);
		_UTL_LOG_I ("----- services -----");
		std::vector<service>::const_iterator iter = services.begin();
		for (; iter != services.end(); ++ iter) {
			_UTL_LOG_I (
				"svcid:[0x%04x] svctype:[0x%02x] [%s]",
				iter->service_id,
				iter->service_type,
				iter->service_name.c_str()
			);
		}
		_UTL_LOG_I ("\n");
	}

	void dump_simple (void) const {
		char s [256] = {0};
		int n = 0;
		std::vector<service>::const_iterator iter = services.begin();
		for (; iter != services.end(); ++ iter) {
			n += snprintf (
				s + n,
				sizeof(s),
				"0x%04x,",
				iter->service_id
			);
		}

		_UTL_LOG_I (
//			"pych:[%d] tsid:[0x%04x] org_nid:[0x%04x] ts_name:[%s] remote_control_key_id:[0x%02x] svc_id:[%s]",
			"pych:[%d] tsid:[0x%04x] org_nid:[0x%04x] ts_name:[%s] remote_control_key_id:[0x%02x]",
			pysical_channel,
			transport_stream_id,
			original_network_id,
			ts_name.c_str(),
			remote_control_key_id
//			remote_control_key_id,
//			s
		);
	}
};


class CChannelManager : public CThreadMgrBase
{
public:
	CChannelManager (char *pszName, uint8_t nQueNum);
	virtual ~CChannelManager (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_channelScan (CThreadMgrIf *pIf);
	void onReq_getPysicalChannelByServiceId (CThreadMgrIf *pIf);
	void onReq_getPysicalChannelByRemoteControlKeyId (CThreadMgrIf *pIf);
	void onReq_tuneByServiceId (CThreadMgrIf *pIf);
	void onReq_tuneByServiceId_withRetry (CThreadMgrIf *pIf);
	void onReq_tuneByRemoteControlKeyId (CThreadMgrIf *pIf);
	void onReq_getChannels (CThreadMgrIf *pIf);
	void onReq_getTransportStreamName (CThreadMgrIf *pIf);
	void onReq_getServiceName (CThreadMgrIf *pIf);
	void onReq_dumpChannels (CThreadMgrIf *pIf);

	void onReceiveNotify (CThreadMgrIf *pIf) override;


private:
	uint16_t getPysicalChannelByServiceId (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	) const ;

	uint16_t getPysicalChannelByRemoteControlKeyId (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint8_t _remote_control_key_id
	) const;

	bool isDuplicateChannel (const CChannel* p_channel) const;

	const CChannel* findChannel (uint16_t pych) const;

	int getChannels (CChannelManagerIf::CHANNEL_t *p_out_channels, int array_max_num) const;
	const char* getTransportStreamName (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id
	) const;
	const char* getServiceName (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	) const;

	void dumpChannels (void) const;
	void dumpChannels_simple (void) const;

	void saveChannels (void);
	void loadChannels (void);


	ST_SEQ_BASE mSeqs [EN_SEQ_CHANNEL_MANAGER__NUM]; // entity


	uint8_t m_tuneCompNotify_clientId;
	EN_PSISI_STATE m_psisiState;

	std::map <uint16_t, CChannel> m_channels; // <pysical channel, CChannel>

};

#endif
