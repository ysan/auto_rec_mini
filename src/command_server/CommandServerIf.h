#ifndef _COMMAND_SERVER_IF_H_
#define _COMMAND_SERVER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"
#include "modules.h"


class CCommandServerIf : public threadmgr::CThreadMgrExternalIf
{
public:
	enum class sequence : int {
		module_up = 0,
		module_down,
		server_loop,
		max,
	};

public:
	explicit CCommandServerIf (CThreadMgrExternalIf *p_if)
		: CThreadMgrExternalIf (p_if)
		, m_module_id (static_cast<uint8_t>(module::module_id::command_server))
	{
	};

	virtual ~CCommandServerIf (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_async (m_module_id, sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (m_module_id, sequence);
	};

private:
	uint8_t m_module_id;
};

#endif
