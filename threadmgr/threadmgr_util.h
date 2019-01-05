#ifndef _THREADMGR_UTIL_H_
#define _THREADMGR_UTIL_H_


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
	EN_LOG_TYPE_I,		// information
	EN_LOG_TYPE_N,		// notice
	EN_LOG_TYPE_W,		// warning
	EN_LOG_TYPE_E,		// error
	EN_LOG_TYPE_PE,		// perror
} EN_LOG_TYPE;


/*
 * Variables
 */
extern FILE *g_fpLog;


/*
 * log macro
 */
/* --- Information --- */
#ifdef _LOG_SIMPLE
#define THM_LOG_I(fmt, ...) {\
	putsLogLW (g_fpLog, EN_LOG_TYPE_I, fmt, ##__VA_ARGS__);\
}
#else
#define THM_LOG_I(fmt, ...) {\
	putsLog (g_fpLog, EN_LOG_TYPE_I, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
}
#endif

/* --- Notice --- */
#ifdef _LOG_SIMPLE
#define THM_LOG_N(fmt, ...) {\
	putsLogLW (g_fpLog, EN_LOG_TYPE_N, fmt, ##__VA_ARGS__);\
}
#else
#define THM_LOG_N(fmt, ...) {\
	putsLog (g_fpLog, EN_LOG_TYPE_N, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
}
#endif

/* --- Warning --- */
#ifdef _LOG_SIMPLE
#define THM_LOG_W(fmt, ...) {\
	putsLogLW (g_fpLog, EN_LOG_TYPE_W, fmt, ##__VA_ARGS__);\
}
#else
#define THM_LOG_W(fmt, ...) {\
	putsLog (g_fpLog, EN_LOG_TYPE_W, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
}
#endif

/* --- Error --- */
#ifdef _LOG_SIMPLE
#define THM_LOG_E(fmt, ...) {\
	putsLogLW (g_fpLog, EN_LOG_TYPE_E, fmt, ##__VA_ARGS__);\
}
#else
#define THM_LOG_E(fmt, ...) {\
	putsLog (g_fpLog, EN_LOG_TYPE_E, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
}
#endif

/* --- perror --- */
#ifdef _LOG_SIMPLE
#define THM_PERROR(fmt, ...) {\
	putsLogLW (g_fpLog, EN_LOG_TYPE_PE, fmt, ##__VA_ARGS__);\
}
#else
#define THM_PERROR(fmt, ...) {\
	putsLog (g_fpLog, EN_LOG_TYPE_PE, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);\
}
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*
 * External
 */
extern void putsSysTime (void);
extern void putsThreadName (void);
extern void getTimeOfDay (struct timeval *p);
extern bool initLog (void);
extern void initLogStdout (void);
extern void finalizLog (void);
extern void setLogFileptr (FILE *p);
extern FILE* getLogFileptr (void);
extern void putsLog (
	FILE *pFp,
	EN_LOG_TYPE enLogType,
	const char *pszFile,
	const char *pszFunc,
	int nLine,
	const char *pszFormat,
	...
);
extern void putsLogLW (
	FILE *pFp,
	EN_LOG_TYPE enLogType,
	const char *pszFormat,
	...
);
extern void deleteLF (char *p);
extern void putsBackTrace (void);

#ifdef __cplusplus
};
#endif

#endif
