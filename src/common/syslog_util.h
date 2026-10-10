#ifndef _SYSLOG_H_
#define _SYSLOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdarg.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <ctime>
#include <sstream>
#include <algorithm>

namespace arec {
namespace common {


// The BSD syslog Protocol
// https://datatracker.ietf.org/doc/html/rfc3164#section-4.1.1

class syslog_util {
private:
	class unix_domain_udp {
	public:
		explicit unix_domain_udp (std::string endpoint)
			: endpoint_ (endpoint)
			, socket_ (-1)
		{
		}

		virtual ~unix_domain_udp (void) {
		}

		bool _open () {
			int s;
			s = socket(AF_UNIX, SOCK_DGRAM, 0);
			if (s < 0) {
				perror("socket");
				return false;
			}
			socket_ = s;
			return true;
		}

		bool _send (uint8_t *p_buff, size_t length) {
			if (socket_ < 0) {
				return false;
			}

			int n = 0;
			int l = length;
			int sent = 0;
			struct sockaddr_un addr;
			memset(&addr, 0, sizeof(struct sockaddr_un));

			addr.sun_family = AF_UNIX;
			strncpy(addr.sun_path, endpoint_.c_str(), endpoint_.length());

			while (sent < (int)length) {
				n = sendto (socket_, p_buff, l, 0, (struct sockaddr *)&addr, sizeof(addr));
				if (n < 0) {
					perror("sendto");
					return false;
				} else {
					sent += n;
					p_buff += n;
					l -= n;
				}
			}

			return true;
		}

		void _close () {
			if (close (socket_) < 0) {
				perror("close");
			}
		}

		int get_socket (void) {
			return socket_;
		}

	private:
		std::string endpoint_;
		int socket_;
	};

//	class CNetUdp {
//	}

public:
	explicit syslog_util (std::string endpoint, int facility, std::string tag, bool with_pid=true)
		: endpoint_(endpoint)
		, unix_udp_(endpoint)
		, facility_ (facility)
		, tag_(tag)
		, with_pid_(with_pid)
	{
		unix_udp_._open();
	}

	int get_fd (void) {
		return unix_udp_.get_socket();
	}
	
	virtual ~syslog_util (void) {
		unix_udp_._close();
	}

	void emergency (const char* format, ...) {
		va_list va;
		va_start(va, format);
		put (LOG_EMERG, format, va);
		va_end(va);
	}

	void alert (const char* format, ...) {
		va_list va;
		va_start(va, format);
		put (LOG_ALERT, format,va);
		va_end(va);
	}

	void critical (const char* format, ...) {
		va_list va;
		va_start(va, format);
		put (LOG_CRIT, format,va);
		va_end(va);
	}

	void error (const char* format, ...) {
		va_list va;
		va_start(va, format);
		put (LOG_ERR, format, va);
		va_end(va);
	}

	void warning (const char* format, ...) {
		va_list va;
		va_start(va, format);
		put (LOG_WARNING, format, va);
		va_end(va);
	}

	void notice (const char* format, ...) {
		va_list va;
		va_start(va, format);
		put (LOG_NOTICE, format, va);
		va_end(va);
	}

	void info (const char* format, ...) {
		va_list va;
		va_start(va, format);
		put (LOG_INFO, format, va);
		va_end(va);
	}

	void debug (const char* format, ...) {
		va_list va;
		va_start(va, format);
		put (LOG_DEBUG, format, va);
		va_end(va);
	}

private:
	int get_priority (int facility, int severity) const {
//		return facility * 8 + severity;
		return facility | severity;
	}

	std::string get_header (void) const {
		std::time_t t = std::time(nullptr);
		char timestamp [64] = {0};
		// format: "Mmm dd hh:mm:ss"
		std::strftime (timestamp, sizeof(timestamp), "%b %e %H:%M:%S", std::localtime(&t));

//		char hostname[128] = {0};
//		gethostname(hostname, sizeof(hostname));

		std::stringstream ss;
		if (with_pid_) {
			pid_t pid = getpid();
//			ss << timestamp << " " << hostname << " " << tag_ << "[" << std::to_string(pid) << "]";
			ss << timestamp << " " << tag_ << "[" << std::to_string(pid) << "]";
		} else {
//			ss << timestamp << " " << hostname << " " << tag_ ;
			ss << timestamp << " " << tag_ ;
		}
		return ss.str();
	}

	void put (int severity, const char* format, va_list &va) {
		char buff[1024] = {0};

		std::string priority = std::to_string(get_priority(facility_, severity));
		int n = snprintf(buff, sizeof(buff), "<%s>%s: ",
					priority.c_str(), get_header().c_str());

		vsnprintf(buff + n, sizeof(buff) - n, format, va);
//std::cout << buff << std::endl;
		if (std::strstr(buff, "\n") == nullptr) {
			unix_udp_._send((uint8_t*)buff, strlen(buff));
		} else {
			auto splited = split (buff + n);
			for (const auto &s : *splited) {
				strncpy(buff + n, s.c_str(), s.length());
				unix_udp_._send((uint8_t*)buff, n + s.length());
			}
		}
	}

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
					r->push_back(""); // Don't push_back if you want to ignore emptiness.
					continue;

				} else if (pos == std::string::npos) {
					std::string sub = s.substr(offset);
					if (sub.length() == 0) {
						// empty (last)
						r->push_back(""); // Don't push_back if you want to ignore emptiness.
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

		// remove tail LF
		auto &v = *r;
		while (1) {
			if (v.size() <= 1) {
				break;
			}
			auto it = std::find_if (v.rbegin(), v.rend(), [](const std::string& s){ return (s.length() == 0); });
			if (it == v.rbegin()) {
				v.erase(it.base());
			} else {
				break;
			}
		}

		return r;
	}

	std::shared_ptr<std::vector<std::string>> split (std::string s) const {
		auto r = split(s, "\n");

		// remove tail LF
		auto &v = *r;
		while (1) {
			if (v.size() <= 1) {
				break;
			}
			auto it = std::find_if (v.rbegin(), v.rend(), [](const std::string& s){ return (s.length() == 0); });
			if (it == v.rbegin()) {
				v.erase(it.base());
			} else {
				break;
			}
		}

		return r;
	}


	std::string endpoint_;
	unix_domain_udp unix_udp_;
	int facility_;
	std::string tag_;
	bool with_pid_;
};


} // namespace common
} // namespace arec

#endif


#if 0
// test code cpp
#include "syslog_util.h"

namespace arec {
namespace common {

int main (void)
{
	syslog s("/dev/log", LOG_USER, "tag_test");

	s.debug("testdeb");
	s.info("testinfo %d", 100);
	s.warning("testwarn %s %s", "a", "rr");
	s.error("testerr %f", 1.00);

	s.info("\ntests %f\n ssss \nrrrrrr\n\n12\n", 1.00);

	return 0;
}

} // namespace common
} // namespace arec

#endif
