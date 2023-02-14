#ifndef _COMMAND_SERVER_H_
#define _COMMAND_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "CommandServerIf.h"
#include "CommandTables.h"
#include "CommandServerLog.h"
#include "Settings.h"


class CCommandServer : public threadmgr::CThreadMgrBase
{
public:
	CCommandServer (std::string name, uint8_t que_max);
	virtual ~CCommandServer (void);


	void on_create (void) override;
	void on_destroy (void) override;

	void on_module_up (threadmgr::CThreadMgrIf *pIf);
	void on_module_down (threadmgr::CThreadMgrIf *pIf);
	void on_server_loop (threadmgr::CThreadMgrIf *pIf);

	static void need_destroy (void);
	static void clear_need_destroy (void);
	static bool is_need_destroy (void);

	int get_client_fd (void);
	void connection_close (void);

private:
	void server_loop (void);
	int recv_parse_delimiter (int fd, char *psz_buff, int buff_size, const char* psz_delim);
	bool parse_delimiter (char *psz_buff, int buff_size, const char *psz_delim);
	void parse_command (char *psz_buff);
	void ignore_sigpipe (void);

	static void print_sub_tables (void);
	static void show_list (const char *psz_desc);
	static void find_command (const char* psz_command, int argc, char *argv[], CThreadMgrBase *p_base);

	// callbacks
	static void on_command_wait_begin (void);
	static void on_command_line_available (const char* psz_command, int argc, char *argv[], CThreadMgrBase *p_base);
	static void on_command_line_through (void);
	static void on_command_wait_end (void);


	int m_clientfd;
	bool m_is_connection_close;

};

#endif
