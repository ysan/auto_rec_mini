#ifndef _FORKER_H_
#define _FORKER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include <stdarg.h>

class CForker {
private:
	class CPipe {
	public:
		CPipe (void) : m_fprintf_cb(fprintf) {
			for (int i = 0; i < 2; ++ i) {
				fd[i] = 0;
				m_is_opened[i] = false;
			}
		}

		virtual ~CPipe (void) {
			destroy();
		}

		bool create (void) {
			if (pipe(fd) < 0) {
				m_fprintf_cb(stderr, "pipe: %s\n", strerror(errno));
				return false;
			}

			m_is_opened[0] = true;
			m_is_opened[1] = true;
			return true;
		}

		void destroy (void) {
			close_reader_fd();
			close_writer_fd();
		}

		int get_reader_fd (void) {
			return fd[0];
		}

		int get_writer_fd (void) {
			return fd[1];
		}

		void close_reader_fd (void) {
			if (m_is_opened[0]) {
				close (fd[0]);
				m_is_opened[0] = false;
			}
		}

		void close_writer_fd (void) {
			if (m_is_opened[1]) {
				close (fd[1]);
				m_is_opened[1] = false;
			}
		}

		void set_log_cb (int (*_cb)(FILE *fp, const char *format, ...)) {
			m_fprintf_cb = _cb;
		}

	private:
		int fd[2];
		bool m_is_opened[2];

		// for log print
		int (*m_fprintf_cb)(FILE *fp, const char *format, ...) ;
	};

public:
	CForker (void)
		: m_child_pid (0)
		, m_fprintf_cb(fprintf)
	{
		m_command.clear();
	}
	
	virtual ~CForker (void) {
		m_command.clear();
		destroy_pipes();
	}

	bool create_pipes (void) {
		if (!m_pipe_p2c.create()) {
			return false;
		}
		if (!m_pipe_c2p_stdout.create()) {
			m_pipe_p2c.destroy();
			return false;
		}
		if (!m_pipe_c2p_stderr.create()) {
			m_pipe_p2c.destroy();
			m_pipe_c2p_stdout.destroy();
			return false;
		}

		return true;
	}

	void destroy_pipes (void) {
		m_pipe_p2c.destroy();
		m_pipe_c2p_stdout.destroy();
		m_pipe_c2p_stderr.destroy();
	}
	
	bool do_fork (std::string command) {
		if (command.length() == 0) {
			m_fprintf_cb(stderr, "command is none\n");
			return false;
		}
		m_command = command;

		auto splited_com = split (command, " ");
		char *_coms[splited_com.size() +1] ;
		m_fprintf_cb (stdout, "execv args\n");
		for (size_t i = 0; i < splited_com.size(); ++ i) {
			_coms [i] = (char*)splited_com[i].c_str();
			m_fprintf_cb (stdout, "  [%s]\n", _coms [i]);
		}
		_coms [splited_com.size()] = NULL;

		pid_t _pid = fork();
		if (_pid < 0) {
			m_fprintf_cb(stderr, "fork: %s\n", strerror(errno));
			destroy_pipes();
			return false;

		} else if (_pid == 0) {
			// child process

			// close impossible fds
			m_pipe_p2c.close_writer_fd();
			m_pipe_c2p_stdout.close_reader_fd();
			m_pipe_c2p_stderr.close_reader_fd();

			// assign stdin
			if (dup2(m_pipe_p2c.get_reader_fd(), STDIN_FILENO) < 0) {
				m_fprintf_cb(stderr, "dup2 (stdin): %s\n", strerror(errno));
				m_pipe_p2c.close_reader_fd();
				m_pipe_c2p_stdout.close_writer_fd();
				m_pipe_c2p_stderr.close_writer_fd();
				return false;
			}
			// assign stdout
			if (dup2(m_pipe_c2p_stdout.get_writer_fd(), STDOUT_FILENO) < 0) {
				m_fprintf_cb(stderr, "dup2 (stdout): %s\n", strerror(errno));
				m_pipe_p2c.close_reader_fd();
				m_pipe_c2p_stdout.close_writer_fd();
				m_pipe_c2p_stderr.close_writer_fd();
				return false;
			}
			// assign stderr
			if (dup2(m_pipe_c2p_stderr.get_writer_fd(), STDERR_FILENO) < 0) {
				m_fprintf_cb(stderr, "dup2 (stderr): %s\n", strerror(errno));
				m_pipe_p2c.close_reader_fd();
				m_pipe_c2p_stdout.close_writer_fd();
				m_pipe_c2p_stderr.close_writer_fd();
				return false;
			}
			m_pipe_p2c.close_reader_fd();
			m_pipe_c2p_stdout.close_writer_fd();
			m_pipe_c2p_stderr.close_writer_fd();

			int r = execv (splited_com[0].c_str(), _coms);
			if (r < 0) {
				m_fprintf_cb(stderr, "execv: %s\n", strerror(errno));
				return false;
			}
		}

		// parent process
		m_child_pid = _pid;
		m_fprintf_cb (stdout, "child process pid: %d\n", m_child_pid);

		// close impossible fds
		m_pipe_p2c.close_reader_fd();
		m_pipe_c2p_stdout.close_writer_fd();
		m_pipe_c2p_stderr.close_writer_fd();

		// available fds
		// m_pipe_p2c.get_writer_fd()        -> get_child_stdin_fd()
		// m_pipe_c2p_stdout.get_reader_fd() -> get_child_stdout_fd()
		// m_pipe_c2p_stderr.get_reader_fd() -> get_child_stderr_fd()

		return true;
	}

	int get_child_stdin_fd (void) {
		return m_pipe_p2c.get_writer_fd();
	}

	int get_child_stdout_fd (void) {
		return m_pipe_c2p_stdout.get_reader_fd();
	}

	int get_child_stderr_fd (void) {
		return m_pipe_c2p_stderr.get_reader_fd();
	}

	void wait_child (int killsignal=-1) {
		if (m_child_pid == 0) {
			m_fprintf_cb(stderr, "not exists child process");
			return ;
		}

		int r = 0;
		int _stat = 0;
		pid_t _r_pid;
		int _cnt = 0;

		while (1) {
			_r_pid = waitpid (m_child_pid, &_stat, WNOHANG);
			if (_r_pid == -1) {
				m_fprintf_cb(stderr, "waitpid: %s\n", strerror(errno));
				break;

			} else if (_r_pid == 0) {
				if (killsignal == -1) {
					// do not kill
					continue;
				}

				if (_cnt < 50) {
					r = kill (m_child_pid, killsignal);
					m_fprintf_cb (stdout, "kill %d [%lu]\n", killsignal, m_child_pid);
				} else {
					r = kill (m_child_pid, SIGKILL);
					m_fprintf_cb (stdout, "kill SIGKILL [%lu]\n", m_child_pid);
				}
				if (r < 0) {
					m_fprintf_cb(stderr, "kill: %s\n", strerror(errno));
				}

			} else {
				if (WIFEXITED(_stat)) {
					m_fprintf_cb (stdout, "child exited with status of [%d]\n", WEXITSTATUS(_stat));
				} else {
					m_fprintf_cb (stdout, "child exited abnromal.\n");
				}
				m_fprintf_cb (stdout, "---> [%s]\n", m_command.c_str());

				m_child_pid = 0;
				break;
			}

			usleep (50000); // 50mS
			++ _cnt;
		}
	}

	void set_log_cb (int (*_cb)(FILE *fp, const char *format, ...)) {
		m_fprintf_cb = _cb;
		m_pipe_p2c.set_log_cb(_cb);
		m_pipe_c2p_stdout.set_log_cb(_cb);
		m_pipe_c2p_stderr.set_log_cb(_cb);
	}

private:
	std::vector<std::string> split (std::string s, std::string sep) const {
		auto r = std::vector<std::string>();

		if (sep.length() == 0) {
			r.push_back(s);

		} else {
			auto offset = std::string::size_type(0);
			while (1) {
				auto pos = s.find(sep, offset);
				if (pos == offset) {
					// ignore empty
					offset = pos + sep.length();
					continue;

				} else if (pos == std::string::npos) {
					std::string sub = s.substr(offset);
					if (sub.length() == 0) {
						// ignore empty (last)
						;;
					} else {
						r.push_back(s.substr(offset));
					}
					break;
				}

				r.push_back(s.substr(offset, pos - offset));
				offset = pos + sep.length();
			}
		}
//		for (const auto &v : r) {
//			std::cout << "[" << v << "]" << std::endl;
//		}

		return std::move(r);
	}

	CPipe m_pipe_p2c;
	CPipe m_pipe_c2p_stdout;
	CPipe m_pipe_c2p_stderr;
	pid_t m_child_pid;
	std::string m_command;

	// for log print
	int (*m_fprintf_cb)(FILE *fp, const char *format, ...) ;
};

#endif


#if 0
// test code cpp
#include <iostream>
#include <sstream>
#include <fstream>

#include "Forker.h"

int main (int argc, char *argv[])
{
	if (argc == 1) {
		puts("invalid arguments.");
		return -1;
	}

	std::cout << "input file: " << argv[1] << std::endl;

	std::ifstream ifs (argv[1], std::ios::in);
	if (!ifs) {
		puts("invalid ifstream.");
		return -1;
	}
	std::stringstream ss;
	ss << ifs.rdbuf();
//	std::cout << ss.str() << std::endl;
	ifs.close();


	CForker forker;
	bool pb = forker.create_pipes();
	printf ("pipe result %d\n", pb);

	bool fb = forker.do_fork(" /usr/bin/timeout 2 /bin/cat ");
	printf ("fork result %d\n", fb);

	write (forker.get_child_stdin_fd(), ss.str().c_str(), ss.str().length());

	int r = 0;
	int fd = forker.get_child_stdout_fd();
	int fd_err = forker.get_child_stderr_fd();
	uint8_t buff [1024] = {0};

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
			perror ("select");
			break;

		} else if (r == 0) {
			// timeout
		}

		// read stdout
		if (FD_ISSET (fd, &_fds)) {
			int _size = read (fd, buff, sizeof(buff));
			if (_size < 0) {
				perror ("read");
				break;

			} else if (_size == 0) {
				// file end
				puts ("stdout read end");
				break;

			} else {
				printf ("stdout test: [%s]", buff);
				fflush(stdout);
			}
		}

		// read stderr
		if (FD_ISSET (fd_err, &_fds)) {
			memset (buff, 0x00, sizeof(buff));
			int _size = read (fd_err, buff, sizeof(buff));
			if (_size < 0) {
				perror ("read");
				break;

			} else if (_size == 0) {
				// file end
				puts ("stderr read end");
				break;

			} else {
				printf ("stderr test: [%s]", buff);
				fflush(stdout);
			}
		}
	}

	forker.wait_child();
	forker.destroy_pipes();

	
	return 0;
}
#endif
