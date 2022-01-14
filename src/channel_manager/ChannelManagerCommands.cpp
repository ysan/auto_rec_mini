#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "ChannelManagerIf.h"
#include "TunerServiceIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "CommandServer.h"
#include "Utils.h"


static void _scan (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CChannelManagerIf _if(pBase->getExternalIf());
	_if.reqChannelScan ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _tune_interactive (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint8_t group_id = 0xff;
	{
		CTunerServiceIf _if(pBase->getExternalIf());
		_if.reqOpenSync ();
		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			group_id = *(uint8_t*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
			_COM_SVR_PRINT ("open: group_id=[%d]\n", group_id);
		} else {
			_COM_SVR_PRINT ("open: failure.\n");
			return;
		}
	}

	CCommandServer* pcs = dynamic_cast <CCommandServer*> (pBase);
	int fd = pcs->getClientFd();

	char buf[32] = {0};


	CChannelManagerIf::CHANNEL_t channels[20] = {0};
	CChannelManagerIf::REQ_CHANNELS_PARAM_t param = {channels, 20};
	CChannelManagerIf _if(pBase->getExternalIf());
	_if.syncGetChannels (&param);
	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_ERROR) {
		_COM_SVR_PRINT ("syncGetChannels is failure.\n");
		return ;
	}

	int ch_num = *(int*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
	_COM_SVR_PRINT ("syncGetChannels ch_num:[%d]\n", ch_num);

	if (ch_num == 0) {
		_COM_SVR_PRINT ("syncGetChannels is 0\n");
		return ;
	}


	// list ts channel
	for (int i = 0; i < 20; ++ i) {
		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			channels[i].transport_stream_id,
			channels[i].original_network_id,
			0 // no need service_id
		};
		CChannelManagerIf _if(pBase->getExternalIf());
		_if.syncGetTransportStreamName (&param);
		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_ERROR) {
			continue ;
		}
		char* ts_name = (char*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, ts_name);
	}
	_COM_SVR_PRINT ("  'q': exit\n");

	// select ts channel
	int sel_ch_num = 0;
	while (1) {
		memset (buf, 0x00, sizeof(buf));
		_COM_SVR_PRINT ("select channel. # ");

		int rSize = CUtils::recvData (fd, (uint8_t*)buf, sizeof(buf), NULL);
		if (rSize <= 0) {
			continue;
		} else {
			CUtils::deleteLF (buf);
			CUtils::deleteHeadSp (buf);
			CUtils::deleteTailSp (buf);

			if (buf[0] == 'q' && strlen(buf) == 1) {
				CTunerServiceIf _if(pBase->getExternalIf());
				_if.reqCloseSync (group_id);
				return;
			}

			std::regex regex_num("^[0-9]+$");
			if (!std::regex_match (buf, regex_num)) {
				continue;
			}

			sel_ch_num = atoi(buf);
			if (sel_ch_num < ch_num) {
				break;
			}
		}
	}

	// list service
	for (int i = 0; i < channels[sel_ch_num].service_num; ++ i) {
		CChannelManagerIf::SERVICE_ID_PARAM_t param = {
			channels[sel_ch_num].transport_stream_id,
			channels[sel_ch_num].original_network_id,
			channels[sel_ch_num].service_ids[i]
		};
		_if.syncGetServiceName (&param);
		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_ERROR) {
			continue ;
		}
		char* svc_name = (char*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, svc_name);
	}
	_COM_SVR_PRINT ("  'q': exit\n");

	// select service
	int sel_svc_num = 0;
	while (1) {
		memset (buf, 0x00, sizeof(buf));
		_COM_SVR_PRINT ("select service. # ");

		int rSize = CUtils::recvData (fd, (uint8_t*)buf, sizeof(buf), NULL);
		if (rSize <= 0) {
			continue;
		} else {
			CUtils::deleteLF (buf);
			CUtils::deleteHeadSp (buf);
			CUtils::deleteTailSp (buf);

			if (buf[0] == 'q' && strlen(buf) == 1) {
				CTunerServiceIf _if(pBase->getExternalIf());
				_if.reqCloseSync (group_id);
				return;
			}

			std::regex regex_num("^[0-9]+$");
			if (!std::regex_match (buf, regex_num)) {
				continue;
			}

			sel_svc_num = atoi(buf);
			if (sel_svc_num < channels[sel_ch_num].service_num) {
				break;
			}
		}
	}


	CTunerServiceIf::tune_advance_param_t tune_param = {
		channels[sel_ch_num].transport_stream_id,
		channels[sel_ch_num].original_network_id,
		channels[sel_ch_num].service_ids[sel_svc_num],
		group_id,
		true
	};


	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
	
	CTunerServiceIf _ts_if(pBase->getExternalIf());
	_ts_if.reqTuneAdvance (&tune_param);
	
	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _tune_stop (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments. (usage: stop {group_id})\n");
		return;
	}

	std::regex regex ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex)) {
		_COM_SVR_PRINT ("invalid arguments. (group_id)\n");
		return;
	}
	uint8_t group_id = atoi(argv[0]);

	{
		CTunerServiceIf _if(pBase->getExternalIf());
		_if.reqTuneStopSync (group_id);
	
		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			_COM_SVR_PRINT ("tune stop success\n");
		} else {
			_COM_SVR_PRINT ("tune stop error\n");
		}
	}
	{
		CTunerServiceIf _if(pBase->getExternalIf());
		_if.reqCloseSync (group_id);
	
		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_SUCCESS) {
			_COM_SVR_PRINT ("close success\n");
		} else {
			_COM_SVR_PRINT ("close error\n");
		}
	}
}

static void _dump_channels (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CChannelManagerIf _if(pBase->getExternalIf());
	_if.reqDumpChannels ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}


ST_COMMAND_INFO g_chManagerCommands [] = { // extern
	{
		"scan",
		"channel scan",
		_scan,
		NULL,
	},
	{
		"t",
		"tune (interactive mode)",
		_tune_interactive,
		NULL,
	},
	{
		"stop",
		"tune stop (usage: stop {group_id})",
		_tune_stop,
		NULL,
	},
	{
		"d",
		"dump channels",
		_dump_channels,
		NULL,
	},
	//-- term --//
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
};

