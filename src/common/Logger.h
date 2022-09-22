#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <ctime>
#include <chrono>
#include <algorithm>

#include "Syslog.h"

#define _LOGGER_TEXT_ATTR_RESET			"\x1B[0m"
#define _LOGGER_TEXT_BOLD_TYPE				"\x1B[1m"
#define _LOGGER_TEXT_UNDER_LINE			"\x1B[4m"
#define _LOGGER_TEXT_REVERSE				"\x1B[7m"
#define _LOGGER_TEXT_BLACK					"\x1B[30m"
#define _LOGGER_TEXT_RED					"\x1B[31m"
#define _LOGGER_TEXT_GREEN					"\x1B[32m"
#define _LOGGER_TEXT_YELLOW				"\x1B[33m"
#define _LOGGER_TEXT_BLUE					"\x1B[34m"
#define _LOGGER_TEXT_MAGENTA				"\x1B[35m"
#define _LOGGER_TEXT_CYAN					"\x1B[36m"
#define _LOGGER_TEXT_WHITE					"\x1B[37m"
#define _LOGGER_TEXT_STANDARD_COLOR		"\x1B[39m"


class CLogger {
private:
	static constexpr int LOG_STRING_SIZE = 1024;
	static constexpr int SYSTIME_STRING_SIZE = 2+1+2+1+2+1+2+1+2 +1;
	static constexpr int SYSTIME_MS_STRING_SIZE = 2+1+2+1+2+1+2+1+2+1+3 +1;
	static constexpr int THREAD_NAME_STRING_SIZE = 10+1;

public:
	enum class level : int {
		debug = 0,
		info,
		warning,
		error,
		perror
	};

public:
	CLogger (void)
		: m_log_level(level::debug)
		, m_syslog(nullptr)
	{
		m_handlers.clear();
	}
	
	virtual ~CLogger (void) {
	}


	void set_syslog (std::shared_ptr<CSyslog> syslog) {
		if (syslog != nullptr) {
			m_syslog = syslog;
		}
	}

	void append_handler (FILE* fp) {
		//TODO lock m_handlers
		m_handlers.push_back(fp);
	}

	void remove_handler (FILE* fp) {
		int i = 0;
		//TODO lock m_handlers
		for (const auto &f : m_handlers) {
			if (f == fp) {
				m_handlers.erase(m_handlers.begin() + i);
				break;
			}
			++ i;
		}
	}

	level get_log_level (void) {
		return m_log_level;
	}

	void set_log_level (level _level) {
		m_log_level = _level;
	}

	void puts (
		level log_level,
		const char *file,
		const char *func,
		int line,
		const char *format,
		...
	)
	{
		if (m_log_level > log_level) {
			return ;
		}

		//TODO lock m_handlers
		for (const auto & fp : m_handlers) {
			va_list va;
			va_start (va, format);
			puts_log (fp, log_level, file, func, line, format, va);
			va_end (va);
		}

		if (m_syslog != nullptr) {
			va_list va;
			va_start (va, format);
			puts_syslog(log_level, file, func, line, format, va);
			va_end (va);
		}
	}

	void puts_l (
		level log_level,
		const char *format,
		...
	)
	{
		if (m_log_level > log_level) {
			return ;
		}

		//TODO lock m_handlers
		for (const auto & fp : m_handlers) {
			va_list va;
			va_start (va, format);
			puts_log_l (fp, log_level, format, va);
			va_end (va);
		}

		if (m_syslog != nullptr) {
			va_list va;
			va_start (va, format);
			puts_syslog_l(log_level, format, va);
			va_end (va);
		}
	}

	void puts_without_header (
		const char *format,
		...
	)
	{
		//TODO lock m_handlers
		for (const auto & fp : m_handlers) {
			va_list va;
			va_start (va, format);
			puts_log_without_header (fp, format, va);
			va_end (va);
		}

		if (m_syslog != nullptr) {
			va_list va;
			va_start (va, format);
			puts_syslog_without_header (format, va);
			va_end (va);
		}
	}


private:
	void puts_log (
		FILE *fp,
		level log_level,
		const char *file,
		const char *func,
		int line,
		const char *format,
		va_list &va
	)
	{
		if (!fp || !file || !func || !format) {
			return ;
		}

		char buff [LOG_STRING_SIZE];
		char time [SYSTIME_MS_STRING_SIZE];
	    char thread_name [THREAD_NAME_STRING_SIZE];
		char type;
		char buff_perror[32];
		char *p_buff_perror = NULL;

		memset (buff, 0x00, sizeof (buff));
		memset (time, 0x00, sizeof (time));
	    memset (thread_name, 0x00, sizeof (thread_name));
		memset (buff_perror, 0x00, sizeof (buff_perror));

		switch (log_level) {
		case level::debug:
			type = 'D';
			break;

		case level::info:
			type = 'I';
			break;

		case level::warning:
			type = 'W';
#ifndef _LOG_COLOR_OFF
			fprintf (fp, _LOGGER_TEXT_BOLD_TYPE);
			fprintf (fp, _LOGGER_TEXT_YELLOW);
#endif
			break;

		case level::error:
			type = 'E';
#ifndef _LOG_COLOR_OFF
			fprintf (fp, _LOGGER_TEXT_UNDER_LINE);
			fprintf (fp, _LOGGER_TEXT_BOLD_TYPE);
			fprintf (fp, _LOGGER_TEXT_RED);
#endif
			break;

		case level::perror:
			type = 'E';
#ifndef _LOG_COLOR_OFF
			fprintf (fp, _LOGGER_TEXT_REVERSE);
			fprintf (fp, _LOGGER_TEXT_BOLD_TYPE);
			fprintf (fp, _LOGGER_TEXT_MAGENTA);
#endif
			p_buff_perror = strerror_r(errno, buff_perror, sizeof (buff_perror));
			break;

		default:
			type = 'D';
			break;
		}

		vsnprintf (buff, sizeof(buff), format, va);

		get_systime_ms (time, SYSTIME_MS_STRING_SIZE);
		get_thread_name (thread_name, THREAD_NAME_STRING_SIZE);

		if (std::strstr(buff, "\n") == nullptr) {
			puts_fprintf (fp, log_level, thread_name, type, time, buff, p_buff_perror, file, func, line);

		} else {
			auto splited = split (buff, "\n");
			for (const auto &s : *splited) {
				puts_fprintf (fp, log_level, thread_name, type, time, s.c_str(), p_buff_perror, file, func, line);
			}
		}

#ifndef _LOG_COLOR_OFF
		fprintf (fp, _LOGGER_TEXT_ATTR_RESET);
#endif
		fflush (fp);
	}

	void puts_fprintf (
		FILE *fp,
		level log_level,
		const char *thread_name,
		char type,
		const char *time,
		const char *buff,
		const char *buff_perror,
		const char *file,
		const char *func,
		int line,
		bool is_need_lf=true
	)
	{
		if (!fp || !time || !buff || !file || !func) {
			return ;
		}

		switch (log_level) {
		case level::perror:
			fprintf (
				fp,
				is_need_lf ? "[%s] %c %s  %s: %s   src=[%s %s()] line=[%d]\n" :
							"[%s] %c %s  %s: %s   src=[%s %s()] line=[%d]",
				thread_name, type, time, buff, buff_perror, file, func, line);
			break;

		case level::debug:
		case level::info:
		case level::warning:
		case level::error:
		default:
			fprintf (
				fp,
				is_need_lf ? "[%s] %c %s  %s   src=[%s %s()] line=[%d]\n":
							"[%s] %c %s  %s   src=[%s %s()] line=[%d]",
				thread_name, type, time, buff, file, func, line);
			break;
		}

		fflush (fp);
	}

	void puts_syslog (
		level log_level,
		const char *file,
		const char *func,
		int line,
		const char *format,
		va_list &va
	)
	{
		if (m_syslog == nullptr) {
			return;
		}
		if (!file || !func || !format) {
			return ;
		}

		char buff [LOG_STRING_SIZE];
		char time [SYSTIME_MS_STRING_SIZE];
	    char thread_name [THREAD_NAME_STRING_SIZE];
		char type;
		char buff_perror[32];
		char *p_buff_perror = NULL;

		memset (buff, 0x00, sizeof (buff));
		memset (time, 0x00, sizeof (time));
	    memset (thread_name, 0x00, sizeof (thread_name));
		memset (buff_perror, 0x00, sizeof (buff_perror));
	
		vsnprintf (buff, sizeof(buff), format, va);

		get_systime_ms (time, SYSTIME_MS_STRING_SIZE);
		get_thread_name (thread_name, THREAD_NAME_STRING_SIZE);

		std::function<void (
			const char *format, const char *thread_name, char type, const char *time, const char *buff,
			const char *file, const char *func, int line
		)> _f;
		std::function<void (
			const char *format, const char *thread_name, char type, const char *time, const char *buff, const char *p_buff_perror,
			const char *file, const char *func, int line
		)> _f_perror;

		switch (log_level) {
		case level::debug:
			type = 'D';
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff,
				const char *file, const char *func, int line
			) {
				m_syslog->debug(format, thread_name, type, time, buff, file, func, line);
			};
			break;

		case level::info:
			type = 'I';
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff,
				const char *file, const char *func, int line
			) {
				m_syslog->info(format, thread_name, type, time, buff, file, func, line);
			};
			break;

		case level::warning:
			type = 'W';
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff,
				const char *file, const char *func, int line
			) {
				m_syslog->warning(format, thread_name, type, time, buff, file, func, line);
			};
			break;

		case level::error:
			type = 'E';
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff,
				const char *file, const char *func, int line
			) {
				m_syslog->error(format, thread_name, type, time, buff, file, func, line);
			};
			break;

		case level::perror:
			type = 'E';
			p_buff_perror = strerror_r(errno, buff_perror, sizeof (buff_perror));
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff,
				const char *file, const char *func, int line
			) {
				m_syslog->critical(format, thread_name, type, time, buff, file, func, line);
			};
			break;

		default:
			break;
		}

		switch (log_level) {
		case level::perror:
			if (std::strstr(buff, "\n") == nullptr) {
				_f_perror (
					"[%s] %c %s  %s: %s   src=[%s %s()] line=[%d]",
					thread_name, type, time, buff, p_buff_perror, file, func, line
				);
			} else {
				auto splited = split (buff, "\n");
				for (const auto &s : *splited) {
					_f_perror (
						"[%s] %c %s  %s: %s   src=[%s %s()] line=[%d]",
						thread_name, type, time, s.c_str(), p_buff_perror, file, func, line);
				}
			}
			break;

		case level::debug:
		case level::info:
		case level::warning:
		case level::error:
		default:
			if (std::strstr(buff, "\n") == nullptr) {
				_f (
					"[%s] %c %s  %s   src=[%s %s()] line=[%d]",
					thread_name, type, time, buff, file, func, line
				);
			} else {
				auto splited = split (buff, "\n");
				for (const auto &s : *splited) {
					_f (
						"[%s] %c %s  %s   src=[%s %s()] line=[%d]",
						thread_name, type, time, s.c_str(), file, func, line);
				}
			}
			break;
		}
	}

	void puts_log_l (
		FILE *fp,
		level log_level,
		const char *format,
		va_list &va
	)
	{
		if (!fp || !format) {
			return ;
		}

		char buff [LOG_STRING_SIZE];
		char time [SYSTIME_MS_STRING_SIZE];
	    char thread_name [THREAD_NAME_STRING_SIZE];
		char type;
		char buff_perror[32];
		char *p_buff_perror = NULL;

		memset (buff, 0x00, sizeof (buff));
		memset (time, 0x00, sizeof (time));
	    memset (thread_name, 0x00, sizeof (thread_name));
		memset (buff_perror, 0x00, sizeof (buff_perror));

		switch (log_level) {
		case level::debug:
			type = 'D';
			break;

		case level::info:
			type = 'I';
			break;

		case level::warning:
			type = 'W';
#ifndef _LOG_COLOR_OFF
			fprintf (fp, _LOGGER_TEXT_BOLD_TYPE);
			fprintf (fp, _LOGGER_TEXT_YELLOW);
#endif
			break;

		case level::error:
			type = 'E';
#ifndef _LOG_COLOR_OFF
			fprintf (fp, _LOGGER_TEXT_UNDER_LINE);
			fprintf (fp, _LOGGER_TEXT_BOLD_TYPE);
			fprintf (fp, _LOGGER_TEXT_RED);
#endif
			break;

		case level::perror:
			type = 'E';
#ifndef _LOG_COLOR_OFF
			fprintf (fp, _LOGGER_TEXT_REVERSE);
			fprintf (fp, _LOGGER_TEXT_BOLD_TYPE);
			fprintf (fp, _LOGGER_TEXT_MAGENTA);
#endif
			p_buff_perror = strerror_r(errno, buff_perror, sizeof (buff_perror));
			break;

		default:
			type = 'D';
			break;
		}

		vsnprintf (buff, sizeof(buff), format, va);

		get_systime_ms (time, SYSTIME_MS_STRING_SIZE);
		get_thread_name (thread_name, THREAD_NAME_STRING_SIZE);

		if (std::strstr(buff, "\n") == nullptr) {
			puts_fprintf_l (fp, log_level, thread_name, type, time, buff, p_buff_perror);

		} else {
			auto splited = split (buff, "\n");
			for (const auto &s : *splited) {
				puts_fprintf_l (fp, log_level, thread_name, type, time, s.c_str(), p_buff_perror);
			}
		}

#ifndef _LOG_COLOR_OFF
		fprintf (fp, _LOGGER_TEXT_ATTR_RESET);
#endif
		fflush (fp);
	}

	void puts_fprintf_l (
		FILE *fp,
		level log_level,
		const char *thread_name,
		char type,
		const char *time,
		const char *buff,
		const char *buff_perror,
		bool is_need_lf=true
	)
	{
		if (!fp || !time || !buff) {
			return ;
		}

		switch (log_level) {
		case level::perror:
			fprintf (
				fp,
				is_need_lf ? "[%s] %c %s  %s: %s\n":
							"[%s] %c %s  %s: %s",
				thread_name, type, time, buff, buff_perror);
			break;

		case level::debug:
		case level::info:
		case level::warning:
		case level::error:
		default:
			fprintf (
				fp,
				is_need_lf ? "[%s] %c %s  %s\n":
							"[%s] %c %s  %s",
				thread_name, type, time, buff);
			break;
		}
	
		fflush (fp);
	}

	void puts_syslog_l (
		level log_level,
		const char *format,
		va_list &va
	)
	{
		if (m_syslog == nullptr) {
			return;
		}
		if (!format) {
			return ;
		}

		char buff [LOG_STRING_SIZE];
		char time [SYSTIME_MS_STRING_SIZE];
	    char thread_name [THREAD_NAME_STRING_SIZE];
		char type;
		char buff_perror[32];
		char *p_buff_perror = NULL;

		memset (buff, 0x00, sizeof (buff));
		memset (time, 0x00, sizeof (time));
	    memset (thread_name, 0x00, sizeof (thread_name));
		memset (buff_perror, 0x00, sizeof (buff_perror));

		vsnprintf (buff, sizeof(buff), format, va);

		get_systime_ms (time, SYSTIME_MS_STRING_SIZE);
		get_thread_name (thread_name, THREAD_NAME_STRING_SIZE);

		std::function<void (
			const char *format, const char *thread_name, char type, const char *time, const char *buff
		)> _f;
		std::function<void (
			const char *format, const char *thread_name, char type, const char *time, const char *buff, const char *p_buff_perror
		)> _f_perror;

		switch (log_level) {
		case level::debug:
			type = 'D';
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff
			) {
				m_syslog->debug(format, thread_name, type, time, buff);
			};
			break;

		case level::info:
			type = 'I';
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff
			) {
				m_syslog->info(format, thread_name, type, time, buff);
			};
			break;

		case level::warning:
			type = 'W';
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff
			) {
				m_syslog->warning(format, thread_name, type, time, buff);
			};
			break;

		case level::error:
			type = 'E';
			_f = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff
			) {
				m_syslog->error(format, thread_name, type, time, buff);
			};
			break;

		case level::perror:
			type = 'E';
			p_buff_perror = strerror_r(errno, buff_perror, sizeof (buff_perror));
			_f_perror = [&] (
				const char *format, const char *thread_name, char type, const char *time, const char *buff, const char *p_buff_perror
			) {
				m_syslog->critical(format, thread_name, type, time, buff, p_buff_perror);
			};
			break;

		default:
			break;
		}

		switch (log_level) {
		case level::perror:
			if (std::strstr(buff, "\n") == nullptr) {
				_f_perror (
					"[%s] %c %s  %s: %s",
					thread_name, type, time, buff, p_buff_perror
				);
			} else {
				auto splited = split (buff, "\n");
				for (const auto &s : *splited) {
					_f_perror (
						"[%s] %c %s  %s: %s",
						thread_name, type, time, s.c_str(), p_buff_perror);
				}
			}
			break;

		case level::debug:
		case level::info:
		case level::warning:
		case level::error:
		default:
			if (std::strstr(buff, "\n") == nullptr) {
				_f (
					"[%s] %c %s  %s",
					thread_name, type, time, buff
				);
			} else {
				auto splited = split (buff, "\n");
				for (const auto &s : *splited) {
					_f (
						"[%s] %c %s  %s",
						thread_name, type, time, s.c_str());
				}
			}
			break;
		}
	}

	void puts_log_without_header (
		FILE *fp,
		const char *format,
		va_list &va
	)
	{
		char buff [LOG_STRING_SIZE];
		memset (buff, 0x00, sizeof (buff));
		vsnprintf (buff, sizeof(buff), format, va);
		fprintf (fp, "%s", buff);
		fflush (fp);
	}

	void puts_syslog_without_header (
		const char *format,
		va_list &va
	)
	{
		if (m_syslog == nullptr) {
			return;
		}
		char buff [LOG_STRING_SIZE];
		memset (buff, 0x00, sizeof (buff));
		vsnprintf (buff, sizeof(buff), format, va);
		m_syslog->info(buff);
	}

	void get_systime_ms (char *buff, size_t size) const {
		std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
		auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count() % 1000;
		time_t time = std::chrono::system_clock::to_time_t(tp);
		struct tm* t = localtime(&time);
		snprintf (
			buff,
			size,
			"%02d/%02d %02d:%02d:%02d.%03ld",
			t->tm_mon+1,
			t->tm_mday,
			t->tm_hour,
			t->tm_min,
			t->tm_sec,
			msec
		);
	}

	void get_thread_name (char *buff, size_t size) const {
		char name[16] = {0};
		memset (name, 0x00, sizeof(name));
		prctl(PR_GET_NAME, name);

		strncpy (buff, name, size -1);
		int len = strlen(buff);
		if (len < THREAD_NAME_STRING_SIZE -1) {
			int i = 0;
			for (i = 0; i < (THREAD_NAME_STRING_SIZE -1 -len); ++ i) {
				*(buff + len + i) = ' ';
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

				} else if(pos == std::string::npos) {
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


	std::vector<FILE*> m_handlers;
	level m_log_level;
	std::shared_ptr<CSyslog> m_syslog;
};

#endif


#if 0
// test code cpp
#include "Logger.h"
int main (void) {
	CLogger logger;

	auto syslog = std::make_shared<CSyslog>("/dev/log", LOG_USER, "test_tag");
	logger.set_syslog(syslog);

	logger.puts(CLogger::level::info, __FILE__, __func__, __LINE__, "init");

	//--
	logger.append_handler(stdout);
	logger.puts(CLogger::level::info, __FILE__, __func__, __LINE__, "appended %d", 100);

	logger.puts(CLogger::level::debug, __FILE__, __func__, __LINE__, "debug\n\n\n");
	logger.puts(CLogger::level::info, __FILE__, __func__, __LINE__, "info\n");
	logger.puts(CLogger::level::warning, __FILE__, __func__, __LINE__, "warning\n");
	logger.puts(CLogger::level::error, __FILE__, __func__, __LINE__, "error\n");
	logger.puts(CLogger::level::error, __FILE__, __func__, __LINE__, "error1\nerror2\n");

	logger.puts(CLogger::level::debug, __FILE__, __func__, __LINE__, "");
	logger.puts(CLogger::level::debug, __FILE__, __func__, __LINE__, "\n");
	logger.puts(CLogger::level::debug, __FILE__, __func__, __LINE__, "\n\n");
	logger.puts(CLogger::level::debug, __FILE__, __func__, __LINE__, "\nuu\n\n");

	logger.remove_handler(stdout);
	logger.puts(CLogger::level::info, __FILE__, __func__, __LINE__, "removed %d", 100);

	//--
	logger.append_handler(stdout);
	logger.puts_l(CLogger::level::info, "appended %d", 100);

	logger.puts_l(CLogger::level::debug, "debug");
	logger.puts_l(CLogger::level::info, "info");
	logger.puts_l(CLogger::level::warning, "warning");
	logger.puts_l(CLogger::level::error, "error");

	logger.remove_handler(stdout);
	logger.puts_l(CLogger::level::info, "removed %d", 100);

	//--
	logger.set_log_level(CLogger::level::warning);
	logger.append_handler(stdout);
	logger.puts_l(CLogger::level::info, "appended %d", 100);

	logger.puts_l(CLogger::level::debug, "debug");
	logger.puts_l(CLogger::level::info, "info");
	logger.puts_l(CLogger::level::warning, "warning");
	logger.puts_l(CLogger::level::error, "error");

	logger.remove_handler(stdout);
	logger.puts_l(CLogger::level::info, "removed %d", 100);

	//--
	logger.set_log_level(CLogger::level::error);
	logger.append_handler(stdout);
	logger.puts_l(CLogger::level::info, "appended %d", 100);

	logger.puts_l(CLogger::level::error, "\nerror debug\n sddd\n\n \nfdf \n \n");

	logger.remove_handler(stdout);
	logger.puts_l(CLogger::level::info, "removed %d", 100);

	return 0;
}
#endif
