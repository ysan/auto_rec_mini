#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <regex>

#include "ChannelManagerIf.h"
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

	CChannelManagerIf mgr(pBase->getExternalIf());
	mgr.reqChannelScan ();

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _tune (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 1) {
		_COM_SVR_PRINT ("invalid arguments.\n");
		return;
	}

	std::regex regex_idx ("^[0-9]+$");
	if (!std::regex_match (argv[0], regex_idx)) {
		_COM_SVR_PRINT ("invalid arguments. (id)");
		return;
	}

//TODO
// 暫定remote_control_key_idだけ渡す
	uint8_t id = atoi(argv[0]);
	CChannelManagerIf::REMOTE_CONTROL_ID_PARAM_t param = {0, 0, id};

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	CChannelManagerIf mgr(pBase->getExternalIf());
	mgr.reqTuneByRemoteControlKeyId (&param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _tune_interactive (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CCommandServer* pcs = dynamic_cast <CCommandServer*> (pBase);
	int fd = pcs->getClientFd();

	char buf[32] = {0};


	CChannelManagerIf::CHANNEL_t channels[20] = {0};
	CChannelManagerIf::REQ_CHANNELS_PARAM_t param = {channels, 20};
	CChannelManagerIf mgr(pBase->getExternalIf());

	mgr.syncGetChannels (&param);
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
		mgr.syncGetTransportStreamName (&param);
		EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
		if (enRslt == EN_THM_RSLT_ERROR) {
			continue ;
		}
		char* ts_name = (char*)(pBase->getIf()->getSrcInfo()->msg.pMsg);
		_COM_SVR_PRINT ("  %2d: [%s]\n", i, ts_name);
	}

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


	CChannelManagerIf::SERVICE_ID_PARAM_t svc_id_param = {
		channels[sel_ch_num].transport_stream_id,
		channels[sel_ch_num].original_network_id,
		channels[sel_ch_num].service_ids[0] // service_id can be anything
	};

	uint32_t opt = pBase->getExternalIf()->getRequestOption ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);

	mgr.reqTuneByServiceId (&svc_id_param);

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	pBase->getExternalIf()->setRequestOption (opt);
}

static void _tune_stop (int argc, char* argv[], CThreadMgrBase *pBase)
{
	if (argc != 0) {
		_COM_SVR_PRINT ("ignore arguments.\n");
	}

	CChannelManagerIf mgr(pBase->getExternalIf());
	mgr.reqTuneStopSync ();

	EN_THM_RSLT enRslt = pBase->getIf()->getSrcInfo()->enRslt;
	if (enRslt == EN_THM_RSLT_SUCCESS) {
		_COM_SVR_PRINT ("tune stop success\n");
	} else {
		_COM_SVR_PRINT ("tune stop error\n");
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

	CChannelManagerIf mgr(pBase->getExternalIf());
	mgr.reqDumpChannels ();

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
		"tr",
		"tune by remote_control_key_id (usage: tr {remote_control_key_id} )",
		_tune,
		NULL,
	},
	{
		"ti",
		"tune (interactive mode)",
		_tune_interactive,
		NULL,
	},
	{
		"stop",
		"tune stop",
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

