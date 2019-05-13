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

class CScanResult {
public:
	CScanResult (void) {
		clear ();
	}
	~CScanResult (void) {
		clear ();
	}


	uint16_t transport_stream_id;
	uint16_t original_network_id;
	std::string network_name;

	uint16_t area_code;
	uint8_t remote_control_key_id;
	std::string ts_name;

	struct service {
		uint16_t service_id;
		uint8_t service_type;
	};
	std::vector <service> services;


	void set (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		const char *psz_network_name,
		uint16_t _area_code,
		uint8_t _remote_control_key_id,
		const char *psz_ts_name,
		struct service _services[],
		int _services_num
	)
	{
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
		transport_stream_id = 0;
		original_network_id = 0;
		network_name.clear();
		area_code = 0;
		remote_control_key_id = 0;
		ts_name.clear();
		services.clear();
	}

	void dump (void) {
		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x]",
			transport_stream_id,
			original_network_id
		);
		_UTL_LOG_I ("network_name:[%s]", network_name.c_str());
		_UTL_LOG_I (
			"area_code:[0x%04x] remote_control_key_id:[0x%04x] ts_name:[%s]",
			area_code,
			remote_control_key_id,
			ts_name.c_str()
		);
		_UTL_LOG_I ("----- services -----");
		std::vector<service>::iterator iter = services.begin();
		for (; iter != services.end(); ++ iter) {
			_UTL_LOG_I (
				"svcid:[0x%04x] svctype:[0x%02x]",
				iter->service_id,
				iter->service_type
			);
		}
	}

	void dump_simple (void) {
		char s [256] = {0};
		int n = 0;
		std::vector<service>::iterator iter = services.begin();
		for (; iter != services.end(); ++ iter) {
			n += snprintf (
				s + n,
				sizeof(s),
				"0x%04x,",
				iter->service_id
			);
		}

		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x] ts_name:[%s] svc_id[%s]",
			transport_stream_id,
			original_network_id,
			ts_name.c_str(),
			s
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
	void onReq_dumpScanResults (CThreadMgrIf *pIf);



private:
	uint16_t getPysicalChannelByServiceId (
		uint16_t _transport_stream_id,
		uint16_t _original_network_id,
		uint16_t _service_id
	);
	void dumpScanResults (void);
	void dumpScanResults_simple (void);

	void saveScanResults (void);
	void loadScanResults (void);


	ST_SEQ_BASE mSeqs [EN_SEQ_CHANNEL_MANAGER__NUM]; // entity


	uint8_t m_tunerNotify_clientId;

	std::map <uint16_t, CScanResult> m_scanResults; // <pysical channel, CScanResult>

};

#endif
