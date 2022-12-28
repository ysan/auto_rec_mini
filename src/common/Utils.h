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

#include "Defs.h"
#include "Etime.h"
#include "Stack.h"
#include "Logger.h"
#include "Shared.h"


/**
 * log macro
 */
#ifndef _ANDROID_BUILD

// --- Debug ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_D(fmt, ...) do {\
	if (CUtils::get_logger()) {CUtils::get_logger()->puts_l (CLogger::level::debug, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_LOG_D(fmt, ...) do {\
	if (CUtils::get_logger()) {CUtils::get_logger()->puts (CLogger::level::debug, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

// --- Information ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_I(fmt, ...) do {\
	if (CUtils::get_logger()) {CUtils::get_logger()->puts_l (CLogger::level::info, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_LOG_I(fmt, ...) do {\
    if (CUtils::get_logger()) {CUtils::get_logger()->puts (CLogger::level::info, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

// --- Warning ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_W(fmt, ...) do {\
	if (CUtils::get_logger()) {CUtils::get_logger()->puts_l (CLogger::level::warning, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_LOG_W(fmt, ...) do {\
    if (CUtils::get_logger()) {CUtils::get_logger()->puts (CLogger::level::warning, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

// --- Error ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_E(fmt, ...) do {\
	if (CUtils::get_logger()) {CUtils::get_logger()->puts_l (CLogger::level::error, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_LOG_E(fmt, ...) do {\
    if (CUtils::get_logger()) {CUtils::get_logger()->puts (CLogger::level::error, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
} while (0)
#endif

// --- perror ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_PERROR(fmt, ...) do {\
	if (CUtils::get_logger()) {CUtils::get_logger()->puts_l (CLogger::level::perror, fmt, ##__VA_ARGS__);}\
} while (0)
#else
#define _UTL_PERROR(fmt, ...) do {\
	if (CUtils::get_logger()) {CUtils::get_logger()->puts (CLogger::level::perror, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);}\
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


class CUtils
{
private:
	CUtils (void);
	virtual ~CUtils (void);

public:
	static int getDiskFreeMB (const char *path);

	static int readFile (int fd, uint8_t *pBuff, size_t nSize);
	static int recvData (int fd, uint8_t *pBuff, int buffSize, bool *p_isDisconnect);

	static void deleteLF (char *p);
	static void deleteHeadSp (char *p);
	static void deleteTailSp (char *p);

	static void putsBackTrace (void);

	static void dumper (const uint8_t *pSrc, int nSrcLen, bool isAddAscii=true);

	static void set_logger (CLogger *logger);
	static CLogger* get_logger (void);

	static void set_shared (CShared<> *shared);
	static CShared<>* get_shared (void);


	class CScopedMutex
	{
	public:
		CScopedMutex (pthread_mutex_t* pMutex) : mpMutex(NULL) {
			if (pMutex) {
				mpMutex = pMutex;
				pthread_mutex_lock (mpMutex);
			}
		}

		~CScopedMutex (void) {
			if (mpMutex) {
				pthread_mutex_unlock (mpMutex);
			}
		}

	private:
		pthread_mutex_t *mpMutex;
	};

private:

	static CLogger *m_logger;
	static CShared<> *m_shared;
};

#endif
