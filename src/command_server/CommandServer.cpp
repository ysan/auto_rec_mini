#include <cstdio>
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

#include "CommandServer.h"
#include "CommandServerIf.h"

#include "ThreadMgrIf.h"
#include "Utils.h"
#include "modules.h"


// callbacks
static void (*_on_command_wait_begin) (void);
static void (*_on_command_line_through) (void);
static void (*_on_command_line_available)(const char* psz_command, int argc, char *argv[], threadmgr::CThreadMgrBase *p_base);
static void (*_on_command_wait_end) (void);


static command_info_t *gp_current_command_table = NULL;


CCommandServer::CCommandServer (std::string name, uint8_t que_max)
	:CThreadMgrBase (name, que_max)
	,m_clientfd (0)
	,m_is_connection_close (false)
{
	const int _max = static_cast<int>(CCommandServerIf::sequence::max);
	threadmgr::sequence_t seqs [_max] = {
		{[&](threadmgr::CThreadMgrIf *p_if){on_module_up(p_if);}, "on_module_up"},
		{[&](threadmgr::CThreadMgrIf *p_if){on_module_down(p_if);}, "on_module_down"},
		{[&](threadmgr::CThreadMgrIf *p_if){on_server_loop(p_if);}, "on_server_loop"},
	};
	set_sequences (seqs, _max);


	_on_command_wait_begin = on_command_wait_begin;
	_on_command_line_through = on_command_line_through;
	_on_command_line_available = on_command_line_available;
	_on_command_wait_end = on_command_wait_end;

	gp_current_command_table = NULL;
//	CCommandServerLog::setFileptr (stdout);
}

CCommandServer::~CCommandServer (void)
{
	reset_sequences();
}


void CCommandServer::on_create (void)
{
	clear_need_destroy();
}

void CCommandServer::on_module_up (threadmgr::CThreadMgrIf *p_if)
{
	uint8_t section_id;
	threadmgr::action act;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	p_if->reply (threadmgr::result::success);


	// mask SIGPIPE
	ignore_sigpipe ();


	threadmgr::request_option::type opt = get_request_option ();
	opt |= REQUEST_OPTION__WITHOUT_REPLY;	
	set_request_option (opt);

	request_async (static_cast<uint8_t>(module::module_id::command_server), static_cast<int>(CCommandServerIf::sequence::server_loop));

	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
	set_request_option (opt);


	section_id = THM_SECT_ID_INIT;
	act = threadmgr::action::done;
	p_if->set_section_id (section_id, act);
}

void CCommandServer::on_module_down (threadmgr::CThreadMgrIf *p_if)
{
	uint8_t section_id;
	threadmgr::action enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

//
// do something
//

	p_if->reply (threadmgr::result::success);

	section_id = THM_SECT_ID_INIT;
	enAct = threadmgr::action::done;
	p_if->set_section_id (section_id, enAct);
}

void CCommandServer::on_server_loop (threadmgr::CThreadMgrIf *p_if)
{
	uint8_t section_id;
	threadmgr::action enAct;
	enum {
		SECTID_ENTRY = THM_SECT_ID_INIT,
		SECTID_END,
	};

	section_id = p_if->get_section_id();
	_UTL_LOG_D ("(%s) section_id %d\n", p_if->get_sequence_name(), section_id);

	p_if->reply (threadmgr::result::success);


	server_loop ();


	section_id = THM_SECT_ID_INIT;
	enAct = threadmgr::action::done;
	p_if->set_section_id (section_id, enAct);
}

int CCommandServer::get_client_fd (void)
{
	return m_clientfd ;
}

void CCommandServer::connection_close (void)
{
	m_is_connection_close = true;
}

void CCommandServer::server_loop (void)
{
	int r = 0;
	int fd_sock_sv = 0;
	char sz_buff [128];
	uint16_t port = 0;
	struct sockaddr_in st_addr_sv;
	struct sockaddr_in st_addr_cl;
	socklen_t addr_len_cl = sizeof(struct sockaddr_in);

	fd_set st_fds;
	struct timeval st_timeout;

	memset (sz_buff, 0x00, sizeof(sz_buff));
	memset (&st_addr_sv, 0x00, sizeof(struct sockaddr_in));
	memset (&st_addr_cl, 0x00, sizeof(struct sockaddr_in));

	port = CSettings::getInstance()->getParams()->getCommandServerPort();
	st_addr_sv.sin_family = AF_INET;
	st_addr_sv.sin_addr.s_addr = htonl (INADDR_ANY);
	st_addr_sv.sin_port = htons (port);


	if ((fd_sock_sv = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		_UTL_PERROR ("socket()");
		return;
	}

	int optval = 1;
	r = setsockopt (fd_sock_sv, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (r < 0) {
		_UTL_PERROR ("setsockopt()");
		close (fd_sock_sv);
		return;
	}

	if (bind(fd_sock_sv, (struct sockaddr*)&st_addr_sv, sizeof(struct sockaddr_in)) < 0) {
		_UTL_PERROR ("bind()");
		close (fd_sock_sv);
		return;
	}

	if (listen (fd_sock_sv, SOMAXCONN) < 0) {
		_UTL_PERROR ("listen()");
		close (fd_sock_sv);
		return;
	}


	while (1) {
//		_UTL_LOG_I ("accept blocking...");

		FD_ZERO (&st_fds);
		FD_SET (fd_sock_sv, &st_fds);
		st_timeout.tv_sec = 1;
		st_timeout.tv_usec = 0;
		r = select (fd_sock_sv+1, &st_fds, NULL, NULL, &st_timeout);
		if (r < 0) {
			_UTL_PERROR ("select()");
			continue;

		} else if (r == 0) {
			// timeout
			if (is_need_destroy()) {
				_UTL_LOG_I ("destroy --> server_loop break\n");
				clear_need_destroy();
				break;
			}
		}

		if (FD_ISSET (fd_sock_sv, &st_fds)) {

			if ((m_clientfd = accept (fd_sock_sv, (struct sockaddr*)&st_addr_cl, &addr_len_cl)) < 0 ) {
				_UTL_PERROR ("accept()");
				close (fd_sock_sv);
				continue;
			}
				
			_UTL_LOG_I (
				"client_addr:[%s] Socket_fd:[%d] --- connected.\n",
				inet_ntoa(st_addr_cl.sin_addr),
				m_clientfd
			);

			// 接続してきたsocketにログ出力をつなぎます
			int fd_copy = dup (m_clientfd);
			FILE *fp = fdopen (fd_copy, "w");
			CUtils::get_logger()->remove_handler(stdout);
			CUtils::get_logger()->append_handler(fp);


			// begin
			if (_on_command_wait_begin) {
				_on_command_wait_begin ();
			}

			while (1) {
				r = recv_parse_delimiter (m_clientfd, sz_buff, sizeof(sz_buff), (char*)"\r\n");
				if (r <= 0) {
					break;
				}
			}

			// end
			if (_on_command_wait_end) {
				_on_command_wait_end ();
			}


			// socket切断にともなってログ出力をstdoutにもどします
			CUtils::get_logger()->remove_handler(fp);
			CUtils::get_logger()->append_handler(stdout);

			fclose (fp);
			close (m_clientfd);

			_UTL_LOG_I (
				"client:[%s] fd:[%d] --- disconnected.\n",
				inet_ntoa(st_addr_cl.sin_addr),
				m_clientfd
			);

			m_clientfd = 0;
		}
	}

	close (fd_sock_sv);
}

int CCommandServer::recv_parse_delimiter (int fd, char *psz_buff, int buff_size, const char* psz_delim)
{
	if (!psz_buff || buff_size <= 0) {
		return -1;
	}
	if (!psz_delim || (int)strlen(psz_delim) == 0) {
		return -1;
	}

	int r_size = 0;
	int r_size_total = 0;

	bool is_parse_remain = false;
	int offset = 0;

	int r = 0;
	fd_set st_fds;
	struct timeval st_timeout;

	while (1) {
		if (is_parse_remain) {
			offset = (int)strlen(psz_buff);
		} else {
			memset (psz_buff, 0x00, buff_size);
			offset = 0;
		}

		FD_ZERO (&st_fds);
		FD_SET (fd, &st_fds);
		st_timeout.tv_sec = 1;
		st_timeout.tv_usec = 0;
		r = select (fd+1, &st_fds, NULL, NULL, &st_timeout);
		if (r < 0) {
			_UTL_PERROR ("select()");
			continue;

		} else if (r == 0) {
			// timeout
			if (m_is_connection_close) {
				_UTL_LOG_I ("connection close --> recv_parse_delimiter break\n");
				m_is_connection_close = false;
				return 0;
			}

			if (is_need_destroy()) {
				_UTL_LOG_I ("destroy --> recv_parse_delimiter break\n");
				break;
			}
		}

		if (FD_ISSET (fd, &st_fds)) {

			r_size = CUtils::recvData (fd, (uint8_t*)psz_buff + offset, buff_size - offset, NULL);
			if (r_size < 0) {
				return -1;

			} else if (r_size == 0) {
				return 0;

			} else {
				// recv ok
				r_size_total += r_size;

				is_parse_remain = parse_delimiter (psz_buff, buff_size, psz_delim);

			}

		}
	}

	return r_size_total;
}

bool CCommandServer::parse_delimiter (char *psz_buff, int buff_size, const char *psz_delim)
{
	if (!psz_buff || (int)strlen(psz_buff) == 0 || buff_size <= 0) {
		return false;
	}
	if (!psz_delim || (int)strlen(psz_delim) == 0) {
		return false;
	}

	bool is_remain = false;
	char *token = NULL;
	char *saveptr = NULL;
	char *s = psz_buff;
	bool is_tail_delim = false;
	int n = 0;

	while (1) {
		token = strtok_r_impl (s, psz_delim, &saveptr, &is_tail_delim);
		if (token == NULL) {
			if (n == 0) {
				// not yet recv delimiter
				// carryover remain
//printf ("=> carryover  psz_buff[%s]\n", psz_buff);
				is_remain = true;

			} else {
				// end
			}
			break;
		}

//printf ("[%s]\n", token);
		if ((int)strlen(token) == 0) {
			// through
			if (_on_command_line_through) {
				_on_command_line_through ();
			}

		} else {
			if ((int)strlen(saveptr) > 0) {
				// got
				// next
				CUtils::deleteHeadSp (token);
				CUtils::deleteTailSp (token);
//printf ("=> GOT - exist next [%s]\n", token);
				parse_command (token);

			} else {
				// got
				// last

				if (is_tail_delim) {
					// tail delimiter
					CUtils::deleteHeadSp (token);
					CUtils::deleteTailSp (token);
//printf ("-> GOT - last tail delimiter [%s]\n", token);
					parse_command (token);

				} else {
					// not yet recv delimiter
					// carryover remain
					strncpy (psz_buff, token, buff_size);
//printf ("=> carryover  psz_buff[%s]\n", psz_buff);
					is_remain = true;
				}
			}
		} 

		s = NULL;
		++ n;
	}

	return is_remain;
}

void CCommandServer::parse_command (char *psz_buff)
{
	if (!psz_buff || ((int)strlen(psz_buff) == 0)) {
		return ;
	}

	const char *psz_delim = (const char*)" ";

//	char sz_tmp [(int)strlen(psz_buff) + 1] = {0};
	char sz_tmp [128] = {0};
	strncpy (sz_tmp, psz_buff, sizeof(sz_tmp));

	// count args
	char *str = sz_tmp;
	char *token = NULL;
	char *saveptr = NULL;
	int n_arg = 0;
	while (1) {
		token = strtok_r(str, psz_delim, &saveptr);
		if (!token) {
			break;
		}
		++ n_arg;
		str = NULL;
	}

	if (n_arg == 0) {
		return;

	} else if (n_arg == 1) {
		if (_on_command_line_available) {
			_on_command_line_available (psz_buff, 0, NULL, this);
		}

	} else {
		// n_arg >= 2
		char *p_command = NULL;
		int argc = n_arg -1;
		char *argv[n_arg -1];

		// parse args
		char *str = psz_buff;
		char *token = NULL;
		char *saveptr = NULL;
		int n = 0;
		while (1) {
			token = strtok_r(str, psz_delim, &saveptr);
			if (!token) {
				break;
			}

			if (n == 0) {
				p_command = token;
			} else {
				argv[n-1] = token;
			}
			++ n;
			str = NULL;
		}

		if (_on_command_line_available) {
			_on_command_line_available (p_command, argc, argv, this);
		}
	}
}

void CCommandServer::ignore_sigpipe (void)
{
	sigset_t sigset;
	sigemptyset (&sigset);
	sigaddset (&sigset, SIGPIPE);
	sigprocmask (SIG_BLOCK, &sigset, NULL);
}

void CCommandServer::need_destroy (void)
{
	fopen("/tmp/_server_loop_destory", "w");
}

void CCommandServer::clear_need_destroy (void)
{
	std::remove ("/tmp/_server_loop_destory");
}

bool CCommandServer::is_need_destroy (void)
{
	FILE *fp = fopen("/tmp/_server_loop_destory", "r");
	return (fp != NULL) ? true: false;
}


static CStack <command_info_t> m_stack;
static CStack <command_info_t> m_stack_sub;

void CCommandServer::print_sub_tables (void)
{
	int sp = m_stack_sub.get_sp();
	if (sp < 1) {
		_COM_SVR_PRINT ("/");
		return;
	}

	for (int i = 0; i < sp; ++ i) {
		command_info_t *p = m_stack_sub.ref (i);
		_COM_SVR_PRINT ("/%s", p->desc);
	}
}

void CCommandServer::show_list (const char *psz_desc)
{
	const command_info_t *p_work_table = gp_current_command_table;

	_COM_SVR_PRINT ("\n  ------ %s ------\n", psz_desc ? psz_desc: "???");

	for (int i = 0; p_work_table->command != NULL; ++ i) {
		_COM_SVR_PRINT ("  %-20s -- %-30s\n", p_work_table->command, p_work_table->desc);
		++ p_work_table;
	}
	_COM_SVR_PRINT ("\n");

	print_sub_tables ();
	_COM_SVR_PRINT (" > ");
}

void CCommandServer::find_command (const char* psz_command, int argc, char *argv[], CThreadMgrBase *p_base)
{
	if (((int)strlen("..") == (int)strlen(psz_command)) && strncmp ("..", psz_command, (int)strlen(psz_command)) == 0) {
		if (argc > 0) {
			_COM_SVR_PRINT ("invalid arguments...\n");
			return;
		}

		command_info_t *p = m_stack.pop ();
		m_stack_sub.pop();
		if (p) {
			gp_current_command_table = p;

			if (gp_current_command_table == g_root_command_table) {
				show_list ("root tables");
			} else{
				show_list (m_stack_sub.peep()->desc);
			}
		}

	} else if ((int)strlen(".") == (int)strlen(psz_command) && strncmp (".", psz_command, (int)strlen(psz_command)) == 0) {
		if (argc > 0) {
			_COM_SVR_PRINT ("invalid arguments...\n");
			return;
		}

		if (gp_current_command_table == g_root_command_table) {
			show_list ("root tables");
		} else{
			show_list (m_stack_sub.peep()->desc);
		}

	} else {
		command_info_t *p_work_table = gp_current_command_table;
		bool is_match = false;

		while (p_work_table->command) {
			if (
				((int)strlen(p_work_table->command) == (int)strlen(psz_command)) &&
				(strncmp (p_work_table->command, psz_command, (int)strlen(psz_command)) == 0)
			) {
				// コマンドが一致した
				is_match = true;

				if (p_work_table->cb_command) {

					// コマンド実行
					(void) (p_work_table->cb_command) (argc, argv, p_base);

				} else {
					// 下位テーブルに移る
					if (p_work_table->next) {
						if (argc > 0) {
							_COM_SVR_PRINT ("invalid arguments...\n");

						} else {
							m_stack.push (gp_current_command_table);
							m_stack_sub.push (p_work_table);
							gp_current_command_table = p_work_table->next;
							show_list (p_work_table->desc);
						}

					} else {
						_COM_SVR_PRINT ("command table registration is invalid...\n");
					}
				}
				break;
			}

			++ p_work_table ;
		}

		if (!is_match) {
			_COM_SVR_PRINT ("invalid command...\n");
		}
	}
}

void CCommandServer::on_command_wait_begin (void)
{
	m_stack.clear();
	m_stack_sub.clear();

	_COM_SVR_PRINT ("###  command line  begin. ###\n");

	gp_current_command_table = g_root_command_table ;
	show_list ("root tables");
}

void CCommandServer::on_command_line_through (void)
{
	print_sub_tables ();
	_COM_SVR_PRINT (" > ");
}

void CCommandServer::on_command_line_available (const char* psz_command, int argc, char *argv[], CThreadMgrBase *p_base)
{
	find_command (psz_command, argc, argv, p_base);
}

void CCommandServer::on_command_wait_end (void)
{
	_COM_SVR_PRINT ("\n###  command line  exit. ###\n");

	m_stack.clear();
	m_stack_sub.clear();
}
