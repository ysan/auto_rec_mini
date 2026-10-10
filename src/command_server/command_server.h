#ifndef _COMMAND_SERVER_H_
#define _COMMAND_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "coseqpp.h"

#include "utils.h"
#include "command_server_if.h"
#include "command_tables.h"
#include "command_server_log.h"
#include "settings.h"


namespace arec {
namespace command_server {

class manager : public coseq::module_base
{
public:
	manager (std::string name, uint8_t que_max);
	virtual ~manager (void);


	void on_module_up (coseq::iface *iface);
	void on_module_down (coseq::iface *iface);
	void on_server_loop (coseq::iface *iface);

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
	static void find_command (const char* psz_command, int argc, char *argv[], coseq::module_base *p_base);

	// callbacks
	static void on_command_wait_begin (void);
	static void on_command_line_available (const char* psz_command, int argc, char *argv[], coseq::module_base *p_base);
	static void on_command_line_through (void);
	static void on_command_wait_end (void);


	int clientfd_;
	bool is_connection_close_;

};

} // namespace command_server
} // namespace arec

#endif
