#ifndef _UTILS_H_
#define _UTILS_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <sys/socket.h>

#include <syslog.h>
#include <sys/statvfs.h>

#ifndef _ANDROID_BUILD
#include <execinfo.h>
#endif

#ifdef _ANDROID_BUILD
#include <android/log.h>
#endif

#include <string>
#include <vector>

#include "defs.h"
#include "etime.h"
#include "stack.h"
#include "logger.h"
#include "shared.h"

namespace arec {
namespace common {



/**
 * log macro
 */
#ifndef _ANDROID_BUILD

// --- Debug ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_D(fmt, ...) do {\
	if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts_l (::arec::common::logger::level::debug, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_LOG_D(fmt, ...) do {\
	if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts (::arec::common::logger::level::debug, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

// --- Information ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_I(fmt, ...) do {\
	if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts_l (::arec::common::logger::level::info, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_LOG_I(fmt, ...) do {\
    if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts (::arec::common::logger::level::info, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

// --- Warning ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_W(fmt, ...) do {\
	if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts_l (::arec::common::logger::level::warning, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_LOG_W(fmt, ...) do {\
    if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts (::arec::common::logger::level::warning, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

// --- Error ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_E(fmt, ...) do {\
	if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts_l (::arec::common::logger::level::error, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_LOG_E(fmt, ...) do {\
    if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts (::arec::common::logger::level::error, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

// --- perror ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_PERROR(fmt, ...) do {\
	if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts_l (::arec::common::logger::level::perror, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_PERROR(fmt, ...) do {\
	if (::arec::common::utils::get_logger()) {::arec::common::utils::get_logger()->puts (::arec::common::logger::level::perror, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

#else // _ANDROID_BUILD

// --- Debug ---
#define _UTL_LOG_D(fmt, ...) do {\
	__android_log_print (ANDROID_LOG_DEBUG, __func__, fmt, ##__VA_ARGS__); \
} while (0)

// --- Information ---
#define _UTL_LOG_I(fmt, ...) do {\
	__android_log_print (ANDROID_LOG_INFO, __func__, fmt, ##__VA_ARGS__); \
} while (0)

// --- Warning ---
#define _UTL_LOG_W(fmt, ...) do {\
	__android_log_print (ANDROID_LOG_WARN, __func__, fmt, ##__VA_ARGS__); \
} while (0)

// --- Error ---
#define _UTL_LOG_E(fmt, ...) do {\
	__android_log_print (ANDROID_LOG_ERROR, __func__, fmt, ##__VA_ARGS__); \
} while (0)

// --- perror ---
#define _UTL_PERROR(fmt, ...) do {\
	__android_log_print (ANDROID_LOG_ERROR, __func__, fmt, ##__VA_ARGS__); \
	char szPerror[32]; \
	strerror_r(errno, szPerror, sizeof (szPerror)); \
	__android_log_print (ANDROID_LOG_ERROR, __func__, "%s", szPerror); \
} while (0)

#endif



extern int backtrace(void **array, int size) __attribute__ ((weak));
extern char **backtrace_symbols(void *const *array, int size) __attribute__ ((weak));
#ifdef _ANDROID_BUILD
extern int bionic_backtrace (void **array, int size);
extern char **bionic_backtrace_symbols (void *const *array, int size);
#endif

extern char * strtok_r_impl (char *str, const char *delim, char **saveptr, bool *is_tail_delim);


class utils
{
private:
	utils (void);
	virtual ~utils (void);

public:
	static int get_disk_free_mb (const char *path);

	static int read_file (int fd, uint8_t *pBuff, size_t nSize);
	static int recv_data (int fd, uint8_t *pBuff, int buffSize, bool *p_isDisconnect);

	static void delete_lf (char *p);
	static void delete_head_sp (char *p);
	static void delete_tail_sp (char *p);

	static void puts_back_trace (void);

	static void dumper (const uint8_t *pSrc, int nSrcLen, bool isAddAscii=true);

	static int makedir (const char* file_path, mode_t mode, bool is_recognized_base_name=false);

	static void set_logger (logger *logger);
	static logger* get_logger (void);

	static void set_shared (shared<> *shared);
	static shared<>* get_shared (void);


	class scoped_mutex
	{
	public:
		scoped_mutex (pthread_mutex_t* pMutex) : mutex_(NULL) {
			if (pMutex) {
				mutex_ = pMutex;
				pthread_mutex_lock (mutex_);
			}
		}

		~scoped_mutex (void) {
			if (mutex_) {
				pthread_mutex_unlock (mutex_);
			}
		}

	private:
		pthread_mutex_t *mutex_;
	};

private:

	static logger *logger_;
	static shared<> *shared_;
};


} // namespace common
} // namespace arec

#endif
