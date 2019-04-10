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

#ifndef _ANDROID_BUILD
#include <execinfo.h>
#endif

#ifdef _ANDROID_BUILD
#include <android/log.h>
#endif

#include "Defs.h"
#include "Etime.h"


#ifndef _LOG_COLOR_OFF
#define _UTL_TEXT_ATTR_RESET			"\x1B[0m"
#define _UTL_TEXT_BOLD_TYPE				"\x1B[1m"
#define _UTL_TEXT_UNDER_LINE			"\x1B[4m"
#define _UTL_TEXT_REVERSE				"\x1B[7m"
#define _UTL_TEXT_BLACK					"\x1B[30m"
#define _UTL_TEXT_RED					"\x1B[31m"
#define _UTL_TEXT_GREEN					"\x1B[32m"
#define _UTL_TEXT_YELLOW				"\x1B[33m"
#define _UTL_TEXT_BLUE					"\x1B[34m"
#define _UTL_TEXT_MAGENTA				"\x1B[35m"
#define _UTL_TEXT_CYAN					"\x1B[36m"
#define _UTL_TEXT_WHITE					"\x1B[37m"
#define _UTL_TEXT_STANDARD_COLOR		"\x1B[39m"
#else
#define _UTL_TEXT_ATTR_RESET			""
#define _UTL_TEXT_BOLD_TYPE				""
#define _UTL_TEXT_UNDER_LINE			""
#define _UTL_TEXT_REVERSE				""
#define _UTL_TEXT_BLACK					""
#define _UTL_TEXT_RED					""
#define _UTL_TEXT_GREEN					""
#define _UTL_TEXT_YELLOW				""
#define _UTL_TEXT_BLUE					""
#define _UTL_TEXT_MAGENTA				""
#define _UTL_TEXT_CYAN					""
#define _UTL_TEXT_WHITE					""
#define _UTL_TEXT_STANDARD_COLOR		""
#endif

#define LOG_PATH	"./"
#define LOG_NAME	"trace"
#define LOG_EXT		"log"

typedef enum {
	EN_LOG_LEVEL_D = 0,		// debug
	EN_LOG_LEVEL_I,			// information
	EN_LOG_LEVEL_W,			// warning
	EN_LOG_LEVEL_E,			// error
	EN_LOG_LEVEL_PE,		// perror
} EN_LOG_LEVEL;


/**
 * log macro
 */
#ifndef _ANDROID_BUILD

// --- Debug ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_D(fmt, ...) do {\
	CUtils::putsLogLW (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_D, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define _UTL_LOG_D(fmt, ...) do {\
	CUtils::putsLog (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_D, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif

// --- Information ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_I(fmt, ...) do {\
	CUtils::putsLogLW (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_I, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define _UTL_LOG_I(fmt, ...) do {\
    CUtils::putsLog (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_I, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif

// --- Warning ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_W(fmt, ...) do {\
	CUtils::putsLogLW (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_W, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define _UTL_LOG_W(fmt, ...) do {\
    CUtils::putsLog (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_W, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif

// --- Error ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_LOG_E(fmt, ...) do {\
	CUtils::putsLogLW (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_E, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define _UTL_LOG_E(fmt, ...) do {\
    CUtils::putsLog (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_E, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif

// --- perror ---
#ifndef _LOG_ADD_FILE_INFO
#define _UTL_PERROR(fmt, ...) do {\
	CUtils::putsLogLW (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_PE, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define _UTL_PERROR(fmt, ...) do {\
	CUtils::putsLog (CUtils::mpfpLog, CUtils::getLogLevel(), EN_LOG_LEVEL_PE, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
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
public:
	static void getTimeOfDay (struct timeval *p);
	static void setThreadName (char *p);
	static void getThreadName (char *pszOut, size_t nSize);


	static FILE *mpfpLog;

	static bool initLog (void);
	static void finalizLog (void);

	static void setLogFileptr (FILE *p);
	static FILE* getLogFileptr (void);

	static void putsLog (
		FILE *pFp,
		EN_LOG_LEVEL enLogLevel,
		const char *pszFile,
		const char *pszFunc,
		int nLine,
		const char *pszFormat,
		...
	);
	static void putsLog (
		FILE *pFp,
		EN_LOG_LEVEL enCurLogLevel,
		EN_LOG_LEVEL enLogLevel,
		const char *pszFile,
		const char *pszFunc,
		int nLine,
		const char *pszFormat,
		...
	);

	static void putsLogLW (
		FILE *pFp,
		EN_LOG_LEVEL enLogLevel,
		const char *pszFormat,
		...
	);
	static void putsLogLW (
		FILE *pFp,
		EN_LOG_LEVEL enCurLogLevel,
		EN_LOG_LEVEL enLogLevel,
		const char *pszFormat,
		...
	);

	static void setLogLevel (EN_LOG_LEVEL enLvl);
	static EN_LOG_LEVEL getLogLevel (void);


	static int readFile (int fd, uint8_t *pBuff, size_t nSize);
	static int recvData (int fd, uint8_t *pBuff, int buffSize, bool *p_isDisconnect);

	static void deleteLF (char *p);
	static void deleteHeadSp (char *p);
	static void deleteTailSp (char *p);

	static void putsBackTrace (void);

	static void dumper (const uint8_t *pSrc, int nSrcLen, bool isAddAscii=true);


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

	static void getSysTime (char *pszOut, size_t nSize);
	static void getSysTimeMs (char *pszOut, size_t nSize);

	static void putsLog (
		FILE *pFp,
		EN_LOG_LEVEL enLogLevel,
		const char *pszFile,
		const char *pszFunc,
		int nLine,
		const char *pszFormat,
		va_list va
	);

	static void putsLogLW (
		FILE *pFp,
		EN_LOG_LEVEL enLogLevel,
		const char *pszFormat,
		va_list va
	);

	static void putsLogFprintf (
		FILE *pFp,
		EN_LOG_LEVEL enLogLevel,
		const char *pszThreadName,
		char type,
		const char *pszTime,
		const char *pszBuf,
		const char *pszPerror,
		const char *pszFile,
		const char *pszFunc,
		int nLine,
		bool isNeedLF=true
	);

	static void putsLogFprintf (
		FILE *pFp,
		EN_LOG_LEVEL enLogLevel,
		const char *pszThreadName,
		char type,
		const char *pszTime,
		const char *pszBuf,
		const char *pszPerror,
		bool isNeedLF=true
	);

};

#endif
