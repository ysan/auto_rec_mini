#ifndef _THREADMGR_UTIL_H_
#define _THREADMGR_UTIL_H_

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
#include <sys/stat.h>

#include <syslog.h>

#include "threadmgr_if.h"

/*
 * Constant define
 */
#define THM_TEXT_ATTR_RESET			"\x1B[0m"
#define THM_TEXT_BOLD_TYPE			"\x1B[1m"
#define THM_TEXT_UNDER_LINE			"\x1B[4m"
#define THM_TEXT_REVERSE			"\x1B[7m"
#define THM_TEXT_BLACK				"\x1B[30m"
#define THM_TEXT_RED				"\x1B[31m"
#define THM_TEXT_GREEN				"\x1B[32m"
#define THM_TEXT_YELLOW				"\x1B[33m"
#define THM_TEXT_BLUE				"\x1B[34m"
#define THM_TEXT_MAGENTA			"\x1B[35m"
#define THM_TEXT_CYAN				"\x1B[36m"
#define THM_TEXT_WHITE				"\x1B[37m"
#define THM_TEXT_STANDARD_COLOR		"\x1B[39m"


#define LOG_PATH	"./"
#define LOG_NAME	"trace"
#define LOG_EXT		"log"


/*
 * Type define
 */
typedef enum {
	EN_LOG_TYPE_D,		// debug
	EN_LOG_TYPE_I,		// information
	EN_LOG_TYPE_W,		// warning
	EN_LOG_TYPE_E,		// error
	EN_LOG_TYPE_PE,		// perror
} EN_LOG_TYPE;


/*
 * Variables
 */
extern FILE *g_fp_log;
extern void (*__puts_log) (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *file,
	const char *func,
	int line,
	const char *format,
	...
);
extern void (*__puts_log_LW) (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *format,
	...
);


/*
 * log macro
 */
/* --- Debug --- */
#ifdef _LOG_SIMPLE
#define THM_LOG_D(fmt, ...) do {\
	__puts_log_LW (g_fp_log, EN_LOG_TYPE_D, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define THM_LOG_D(fmt, ...) do {\
	__puts_log (g_fp_log, EN_LOG_TYPE_D, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif

/* --- Information --- */
#ifdef _LOG_SIMPLE
#define THM_LOG_I(fmt, ...) do {\
	__puts_log_LW (g_fp_log, EN_LOG_TYPE_I, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define THM_LOG_I(fmt, ...) do {\
	__puts_log (g_fp_log, EN_LOG_TYPE_I, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif

/* --- Warning --- */
#ifdef _LOG_SIMPLE
#define THM_LOG_W(fmt, ...) do {\
	__puts_log_LW (g_fp_log, EN_LOG_TYPE_W, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define THM_LOG_W(fmt, ...) do {\
	__puts_log (g_fp_log, EN_LOG_TYPE_W, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif

/* --- Error --- */
#ifdef _LOG_SIMPLE
#define THM_LOG_E(fmt, ...) do {\
	__puts_log_LW (g_fp_log, EN_LOG_TYPE_E, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define THM_LOG_E(fmt, ...) do {\
	__puts_log (g_fp_log, EN_LOG_TYPE_E, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif

/* --- perror --- */
#ifdef _LOG_SIMPLE
#define THM_PERROR(fmt, ...) do {\
	__puts_log_LW (g_fp_log, EN_LOG_TYPE_PE, fmt, ##__VA_ARGS__);\
} while (0)
#else
#define THM_PERROR(fmt, ...) do {\
	__puts_log (g_fp_log, EN_LOG_TYPE_PE, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
} while (0)
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*
 * External
 */
extern void puts_sys_time (void);
extern void puts_thread_name (void);
extern void get_thread_name (char *out, size_t size);
extern void get_time_of_day (struct timeval *p);
extern void set_log_fileptr (FILE *p);
extern FILE* get_log_fileptr (void);
extern void set_alternative_log (void (*_fn)(
		FILE *p_fp,
		EN_LOG_TYPE log_type,
		const char *file,
		const char *func,
		int line,
		const char *format,
		...
	)
);
extern void puts_log (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *file,
	const char *func,
	int line,
	const char *format,
	...
);
extern void set_alternative_log_LW (void (*_fn)(
		FILE *p_fp,
		EN_LOG_TYPE log_type,
		const char *format,
		...
	)
);
extern void puts_log_LW (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *format,
	...
);
extern void delete_LF (char *p);
extern void puts_backtrace (void);

#ifdef __cplusplus
};
#endif

#endif
