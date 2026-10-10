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

namespace arec {
namespace common {


class forker {
private:
	class pipe {
	public:
		explicit pipe (void) : fprintf_cb_(nullptr) {
			for (int i = 0; i < 2; ++ i) {
				fd[i] = 0;
				is_opened_[i] = false;
			}
		}

		virtual ~pipe (void) {
			destroy();
		}

		bool create (void) {
			if (::pipe(fd) < 0) {
				if (fprintf_cb_) fprintf_cb_(stderr, "pipe: %s\n", strerror(errno));
				return false;
			}

			is_opened_[0] = true;
			is_opened_[1] = true;
			return true;
		}

		void destroy (void) {
			close_reader_fd();
			close_writer_fd();
		}

		int get_reader_fd (void) const {
			return fd[0];
		}

		int get_writer_fd (void) const {
			return fd[1];
		}

		void close_reader_fd (void) {
			if (is_opened_[0]) {
				close (fd[0]);
				is_opened_[0] = false;
			}
		}

		void close_writer_fd (void) {
			if (is_opened_[1]) {
				close (fd[1]);
				is_opened_[1] = false;
			}
		}

		void set_log_cb (int (*_cb)(FILE *fp, const char *format, ...)) {
			if (fprintf_cb_) fprintf_cb_ = _cb;
		}

	private:
		int fd[2];
		bool is_opened_[2];

		// for log print
		int (*fprintf_cb_)(FILE *fp, const char *format, ...) ;
	};

public:
	class child_status {
	public:
		child_status (int status, int return_code)
			: status_(status)
			, return_code_(return_code)
		{
		}
		virtual ~child_status (void) = default;

		// ret -1 : abromal function,
		//      0 : child alive,
		//      1 : child normal end,
		//      2 : child abrobal end (by signal etc...)
		int get_status (void) const {
			return status_;
		}

		int get_return_code (void) const {
			return return_code_;
		}

	private:
		int status_;
		int return_code_;
	};

public:
	forker (void)
		: child_pid_ (0)
		, fprintf_cb_(nullptr)
	{
		child_commandline_.clear();
	}
	
	virtual ~forker (void) {
		child_commandline_.clear();
		destroy_pipes();
	}

	bool create_pipes (void) {
		if (!pipe_p2c_stdin_.create()) {
			return false;
		}
		if (!pipe_c2p_stdout_.create()) {
			pipe_p2c_stdin_.destroy();
			return false;
		}
		if (!pipe_c2p_stderr_.create()) {
			pipe_p2c_stdin_.destroy();
			pipe_c2p_stdout_.destroy();
			return false;
		}

		return true;
	}

	void destroy_pipes (void) {
		pipe_p2c_stdin_.destroy();
		pipe_c2p_stdout_.destroy();
		pipe_c2p_stderr_.destroy();
	}
	
	bool do_fork (std::string command) {
		if (command.length() == 0) {
			if (fprintf_cb_) fprintf_cb_(stderr, "command is none\n");
			return false;
		}
		child_commandline_ = command;

		auto splited = split (command, " ");
		char *_coms[splited->size() +1] ;
		if (fprintf_cb_) fprintf_cb_ (stdout, "execv args\n");
		for (size_t i = 0; i < splited->size(); ++ i) {
			const auto &vec = *splited;
			_coms [i] = (char*)vec[i].c_str();
			if (fprintf_cb_) fprintf_cb_ (stdout, "  [%s]\n", _coms [i]);
		}
		_coms [splited->size()] = NULL;

		pid_t _pid = fork();
		if (_pid < 0) {
			if (fprintf_cb_) fprintf_cb_(stderr, "fork: %s\n", strerror(errno));
			destroy_pipes();
			return false;

		} else if (_pid == 0) {
			// child process

			// close impossible fds
			pipe_p2c_stdin_.close_writer_fd();
			pipe_c2p_stdout_.close_reader_fd();
			pipe_c2p_stderr_.close_reader_fd();

			// assign stdin
			if (dup2(pipe_p2c_stdin_.get_reader_fd(), STDIN_FILENO) < 0) {
				if (fprintf_cb_) fprintf_cb_(stderr, "dup2 (stdin): %s\n", strerror(errno));
				pipe_p2c_stdin_.close_reader_fd();
				pipe_c2p_stdout_.close_writer_fd();
				pipe_c2p_stderr_.close_writer_fd();
				exit (EXIT_FAILURE);
			}
			// assign stdout
			if (dup2(pipe_c2p_stdout_.get_writer_fd(), STDOUT_FILENO) < 0) {
				if (fprintf_cb_) fprintf_cb_(stderr, "dup2 (stdout): %s\n", strerror(errno));
				pipe_p2c_stdin_.close_reader_fd();
				pipe_c2p_stdout_.close_writer_fd();
				pipe_c2p_stderr_.close_writer_fd();
				exit (EXIT_FAILURE);
			}
			// assign stderr
			if (dup2(pipe_c2p_stderr_.get_writer_fd(), STDERR_FILENO) < 0) {
				if (fprintf_cb_) fprintf_cb_(stderr, "dup2 (stderr): %s\n", strerror(errno));
				pipe_p2c_stdin_.close_reader_fd();
				pipe_c2p_stdout_.close_writer_fd();
				pipe_c2p_stderr_.close_writer_fd();
				exit (EXIT_FAILURE);
			}
			pipe_p2c_stdin_.close_reader_fd();
			pipe_c2p_stdout_.close_writer_fd();
			pipe_c2p_stderr_.close_writer_fd();

			const auto vec = *splited;
			int r = execv (_coms[0], _coms);
			if (r < 0) {
				if (fprintf_cb_) fprintf_cb_(stderr, "execv: %s\n", strerror(errno));
				exit (EXIT_FAILURE);
			}
		}

		// parent process
		child_pid_ = _pid;
		if (fprintf_cb_) fprintf_cb_ (stdout, "child process pid: %d\n", child_pid_);

		// close impossible fds
		pipe_p2c_stdin_.close_reader_fd();
		pipe_c2p_stdout_.close_writer_fd();
		pipe_c2p_stderr_.close_writer_fd();

		// available fds
		// pipe_p2c_stdin_.get_writer_fd()        -> get_child_stdin_fd()
		// pipe_c2p_stdout_.get_reader_fd() -> get_child_stdout_fd()
		// pipe_c2p_stderr_.get_reader_fd() -> get_child_stderr_fd()

		return true;
	}

	pid_t get_child_pid (void) const {
		return child_pid_;
	}

	int get_child_stdin_fd (void) const {
		return pipe_p2c_stdin_.get_writer_fd();
	}

	int get_child_stdout_fd (void) const {
		return pipe_c2p_stdout_.get_reader_fd();
	}

	int get_child_stderr_fd (void) const {
		return pipe_c2p_stderr_.get_reader_fd();
	}

	bool has_child (void) const {
		return child_pid_ != 0 ? true : false;
	}

	bool is_alive_child (void) const {
		if (child_pid_ == 0) {
			return false;
		}
		int r = kill (child_pid_, 0);
		if (r == 0) {
			return true;
		} else {
			return false;
		}
	}

	const std::string &get_child_commandline (void) const {
		return child_commandline_;
	}

	child_status wait_child (int killsignal=-1, bool is_oneshot=false) {
		if (child_pid_ == 0) {
			if (fprintf_cb_) fprintf_cb_(stderr, "not exists child process\n");
			child_status cs{-1, 0};
			return cs;
		}

		int _status = 0;
		int _child_return_code = 0;

		int r = 0;
		int _stat = 0;
		pid_t _r_pid;
		int _cnt = 0;

		while (1) {
			_r_pid = waitpid (child_pid_, &_stat, WNOHANG);
			if (_r_pid == -1) {
				if (fprintf_cb_) fprintf_cb_(stderr, "waitpid: %s\n", strerror(errno));
				_status = -1;
				break;

			} else if (_r_pid == 0) {
				// alive
				if (killsignal == -1) {
					// do not kill
				} else {
					if (_cnt < 50) {
						r = kill (child_pid_, killsignal);
						if (fprintf_cb_) fprintf_cb_ (stdout, "kill %d [%lu]\n", killsignal, child_pid_);
					} else {
						r = kill (child_pid_, SIGKILL);
						if (fprintf_cb_) fprintf_cb_ (stdout, "kill SIGKILL [%lu]\n", child_pid_);
					}
					if (r < 0) {
						if (fprintf_cb_) fprintf_cb_(stderr, "kill: %s\n", strerror(errno));
					}
				}

			} else {
				// not alive
				if (WIFEXITED(_stat)) {
					_status = 1;
					_child_return_code = WEXITSTATUS(_stat);
					if (fprintf_cb_) fprintf_cb_ (stdout, "child exited with status of [%d]\n", _child_return_code);
				} else {
					_status = 2;
					if (fprintf_cb_) fprintf_cb_ (stdout, "child exited abnromal.\n");
				}
				if (fprintf_cb_) fprintf_cb_ (stdout, "---> [%s]\n", child_commandline_.c_str());

				// ** clear child_pid_ **
				child_pid_ = 0;
				child_commandline_.clear();
				break;
			}

			if (is_oneshot) {
				break;
			}

			usleep (50000); // 50mS
			++ _cnt;
		}

		child_status cs {_status, _child_return_code};
		return cs;
	}

	void set_log_cb (int (*_cb)(FILE *fp, const char *format, ...)) {
		fprintf_cb_ = _cb;
		pipe_p2c_stdin_.set_log_cb(_cb);
		pipe_c2p_stdout_.set_log_cb(_cb);
		pipe_c2p_stderr_.set_log_cb(_cb);
	}

private:
	std::shared_ptr<std::vector<std::string>> split (std::string s, std::string sep) const {
		auto r = std::make_shared<std::vector<std::string>>();

		if (sep.length() == 0) {
			r->push_back(s);

		} else {
			auto offset = std::string::size_type(0);
			while (1) {
				auto pos = s.find(sep, offset);
				if (pos == offset) {
					// empty
					offset = pos + sep.length();
//					r->push_back(""); // Don't push_back if you want to ignore emptiness.
					continue;

				} else if (pos == std::string::npos) {
					std::string sub = s.substr(offset);
					if (sub.length() == 0) {
						// empty (last)
//						r->push_back(""); // Don't push_back if you want to ignore emptiness.
					} else {
						r->push_back(s.substr(offset));
					}
					break;
				}

				r->push_back(s.substr(offset, pos - offset));
				offset = pos + sep.length();
			}
		}
//		for (const auto &v : *r) {
//			std::cout << "[" << v << "]" << std::endl;
//		}

		return r;
	}

	pipe pipe_p2c_stdin_;
	pipe pipe_c2p_stdout_;
	pipe pipe_c2p_stderr_;
	pid_t child_pid_;
	std::string child_commandline_;

	// for log print
	int (*fprintf_cb_)(FILE *fp, const char *format, ...) ;
};


} // namespace common
} // namespace arec

#endif


#if 0
// test code cpp
#include <iostream>
#include <sstream>
#include <fstream>

#include "forker.h"

namespace arec {
namespace common {


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


	forker forker;
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

} // namespace common
} // namespace arec

#endif
