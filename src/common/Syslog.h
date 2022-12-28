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

// The BSD syslog Protocol
// https://datatracker.ietf.org/doc/html/rfc3164#section-4.1.1

class CSyslog {
private:
	class CUnixDomainUdp {
	public:
		explicit CUnixDomainUdp (std::string endpoint)
			: m_endpoint (endpoint)
			, m_socket (-1)
		{
		}

		virtual ~CUnixDomainUdp (void) {
		}

		bool _open () {
			int s;
			s = socket(AF_UNIX, SOCK_DGRAM, 0);
			if (s < 0) {
				perror("socket");
				return false;
			}
			m_socket = s;
			return true;
		}

		bool _send (uint8_t *p_buff, size_t length) {
			if (m_socket < 0) {
				return false;
			}

			int n = 0;
			int l = length;
			int sent = 0;
			struct sockaddr_un addr;
			memset(&addr, 0, sizeof(struct sockaddr_un));

			addr.sun_family = AF_UNIX;
			strncpy(addr.sun_path, m_endpoint.c_str(), m_endpoint.length());

			while (sent < (int)length) {
				n = sendto (m_socket, p_buff, l, 0, (struct sockaddr *)&addr, sizeof(addr));
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
			if (close (m_socket) < 0) {
				perror("close");
			}
		}

		int get_socket (void) {
			return m_socket;
		}

	private:
		std::string m_endpoint;
		int m_socket;
	};

//	class CNetUdp {
//	}

public:
	explicit CSyslog (std::string endpoint, int facility, std::string tag, bool with_pid=true)
		: m_endpoint(endpoint)
		, m_unix_udp(endpoint)
		, m_facility (facility)
		, m_tag(tag)
		, m_with_pid(with_pid)
	{
		m_unix_udp._open();
	}

	int get_fd (void) {
		return m_unix_udp.get_socket();
	}
	
	virtual ~CSyslog (void) {
		m_unix_udp._close();
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
		if (m_with_pid) {
			pid_t pid = getpid();
//			ss << timestamp << " " << hostname << " " << m_tag << "[" << std::to_string(pid) << "]";
			ss << timestamp << " " << m_tag << "[" << std::to_string(pid) << "]";
		} else {
//			ss << timestamp << " " << hostname << " " << m_tag ;
			ss << timestamp << " " << m_tag ;
		}
		return ss.str();
	}

	void put (int severity, const char* format, va_list &va) {
		char buff[1024] = {0};

		std::string priority = std::to_string(get_priority(m_facility, severity));
		int n = snprintf(buff, sizeof(buff), "<%s>%s: ",
					priority.c_str(), get_header().c_str());

		vsnprintf(buff + n, sizeof(buff) - n, format, va);
//std::cout << buff << std::endl;
		if (std::strstr(buff, "\n") == nullptr) {
			m_unix_udp._send((uint8_t*)buff, strlen(buff));
		} else {
			auto splited = split (buff + n);
			for (const auto &s : *splited) {
				strncpy(buff + n, s.c_str(), s.length());
				m_unix_udp._send((uint8_t*)buff, n + s.length());
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


	std::string m_endpoint;
	CUnixDomainUdp m_unix_udp;
	int m_facility;
	std::string m_tag;
	bool m_with_pid;
};

#endif


#if 0
// test code cpp
#include "Syslog.h"
int main (void)
{
	CSyslog s("/dev/log", LOG_USER, "tag_test");

	s.debug("testdeb");
	s.info("testinfo %d", 100);
	s.warning("testwarn %s %s", "a", "rr");
	s.error("testerr %f", 1.00);

	s.info("\ntests %f\n ssss \nrrrrrr\n\n12\n", 1.00);

	return 0;
}
#endif
