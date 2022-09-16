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
#include <vector>
#include <memory>
#include <ctime>
#include <sstream>

// The BSD syslog Protocol
// https://datatracker.ietf.org/doc/html/rfc3164#section-4.1.1

class CSyslog {
private:
	class CUnixDomainUdp {
	public:
		CUnixDomainUdp (std::string endpoint)
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
			int n = 0;
			int l = length;
			int sent = 0;
			struct sockaddr_un addr;
			memset(&addr, 0, sizeof(struct sockaddr_un));

			addr.sun_family = AF_UNIX;
			strncpy(addr.sun_path, m_endpoint.c_str(), m_endpoint.length());

			while (sent < length) {
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

		std::string m_endpoint;
		int m_socket;
	};

//	class CNetUdp {}

public:
	CSyslog (std::string endpoint, int facility, std::string tag, bool with_pid=true)
		: m_endpoint(endpoint)
		, m_unix_udp(endpoint)
		, m_facility (facility)
		, m_tag(tag)
		, m_with_pid(with_pid)
	{
		m_unix_udp._open();
	}
	
	virtual ~CSyslog (void) {
		m_unix_udp._close();
	}

	void emergency (const char* format, ...) {
		char buff[1024] = {0};
		va_list va;
		va_start(va, format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		put (LOG_EMERG, buff);
	}

	void alert (const char* format, ...) {
		char buff[1024] = {0};
		va_list va;
		va_start(va, format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		put (LOG_ALERT, buff);
	}

	void critical (const char* format, ...) {
		char buff[1024] = {0};
		va_list va;
		va_start(va, format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		put (LOG_CRIT, buff);
	}

	void error (const char* format, ...) {
		char buff[1024] = {0};
		va_list va;
		va_start(va, format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		put (LOG_ERR, buff);
	}

	void warning (const char* format, ...) {
		char buff[1024] = {0};
		va_list va;
		va_start(va, format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		put (LOG_WARNING, buff);
	}

	void notice (const char* format, ...) {
		char buff[1024] = {0};
		va_list va;
		va_start(va, format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		put (LOG_NOTICE, buff);
	}

	void info (const char* format, ...) {
		char buff[1024] = {0};
		va_list va;
		va_start(va, format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		put (LOG_INFO, buff);
	}

	void debug (const char* format, ...) {
		char buff[1024] = {0};
		va_list va;
		va_start(va, format);
		vsnprintf(buff, sizeof(buff), format, va);
		va_end(va);
		put (LOG_DEBUG, buff);
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

	void put (int severity, const char *p_msg) {
		char buff[1024] = {0};
		std::string priority = std::to_string(get_priority(m_facility, severity));

		snprintf(buff, sizeof(buff), "<%s>%s: %s",
				priority.c_str(), get_header().c_str(), p_msg);
std::cout << buff << std::endl;

		m_unix_udp._send((uint8_t*)buff, strlen(buff));
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
	CSyslog s("/dev/log", LOG_USER, "tag_a");
	s.debug("testdeb");
	s.info("testinfo %d", 100);
	s.warning("testwarn %s %s", "a", "rr");
	s.error("testerr %f", 1.00);
	return 0;
}
#endif
