#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "threadmgr_util.h"


/*
 * Constant define
 */
#define SYSTIME_STRING_SIZE			(2+1+2+1+2+1+2+1+2 +1)
#define SYSTIME_MS_STRING_SIZE		(2+1+2+1+2+1+2+1+2+1+3 +1)
#define THREAD_NAME_STRING_SIZE		(10+1)
#define LOG_STRING_SIZE				(256)
#define BACKTRACE_BUFF_SIZE			(20)

/*
 * Type define
 */

/*
 * Variables
 */
FILE *g_fp_log = NULL;
void (*__puts_log) (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *file,
	const char *func,
	int line,
	const char *format,
	...
) = puts_log;
void (*__puts_log_LW) (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *format,
	...
) = puts_log_LW;

/*
 * Prototypes
 */
static char * strtok_r_impl (char *str, const char *delim, char **saveptr);
void puts_sys_time (void); // extern
static void get_sys_time (char *out, size_t size);
static void get_sys_time_ms (char *out, size_t size);
void puts_thread_name (void); // extern
void get_thread_name (char *out, size_t size); // extern
void get_time_of_day (struct timeval *p); //extern
bool init_log (void); // extern
void init_log_stdout (void); // extern
void finaliz_log (void); // extern
void set_log_fileptr (FILE *p); // extern
FILE* get_log_fileptr (void);
void set_alternative_log (void (*_fn)( // extern
		FILE *p_fp,
		EN_LOG_TYPE log_type,
		const char *file,
		const char *func,
		int line,
		const char *format,
		...
	)
);
void puts_log ( // extern
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *file,
	const char *func,
	int line,
	const char *format,
	...
);
static void puts_log_inner (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *file,
	const char *func,
	int line,
	const char *format,
	va_list va
);
static void puts_log_fprintf (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *thread_name,
	char type,
	const char *time,
	const char *buf,
	const char *perror,
	const char *file,
	const char *func,
	int line
);
void set_alternative_log_LW (void (*_fn)( // extern
		FILE *p_fp,
		EN_LOG_TYPE log_type,
		const char *format,
		...
	)
);
void puts_log_LW ( // extern
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *format,
	...
);
static void puts_log_inner_LW (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *format,
	va_list va
);
static void puts_log_fprintf_LW (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *thread_name,
	char type,
	const char *time,
	const char *buf,
	const char *perror
);


char * strtok_r_impl (char *str, const char *delim, char **saveptr)
{
	if ((!delim) || (strlen(delim) == 0)) {
		return NULL;
	}

	if (!saveptr) {
		return NULL;
	}

	if (str) {

		char *f = strstr (str, delim);
		if (!f) {
			return NULL;
		}

		int i = 0;
		for (i = 0; i < (int)strlen(delim); ++ i) {
			*(f+i) = 0x00;
		}

		*saveptr = f + (int)strlen(delim);
//		printf ("a %p\n", *saveptr);

		return str;

	} else {

		char *r = *saveptr;
		char *f = strstr (*saveptr, delim);
		if (!f) {
			if ((int)strlen(*saveptr) > 0) {
				*saveptr += (int)strlen(*saveptr);
				return r;
			} else {
				return NULL;
			}
		}

		int i = 0;
		for (i = 0; i < (int)strlen(delim); ++ i) {
			*(f+i) = 0x00;
		}

		*saveptr = f + (int)strlen(delim);
//		printf ("b %p\n", *saveptr);

		return r;
	}
}

/**
 * システム現在時刻を出力
 * ログマクロ用
 */
void puts_sys_time (void)
{
	char time [SYSTIME_STRING_SIZE];
	memset (time, 0x00, SYSTIME_STRING_SIZE);
	get_sys_time (time, sizeof (time));
	fprintf (stdout, "%s", time);
}

/**
 * システム現在時刻を取得
 * MM/dd HH:mm:ss形式
 */
static void get_sys_time (char *out, size_t size)
{
	time_t timer;
	struct tm *pst_tm_local = NULL;
	struct tm st_tm_local_tmp;

	timer = time (NULL);
	pst_tm_local = localtime_r (&timer, &st_tm_local_tmp); /* スレッドセーフ */

	snprintf (
		out,
		size,
		"%02d/%02d %02d:%02d:%02d",
		pst_tm_local->tm_mon+1,
		pst_tm_local->tm_mday,
		pst_tm_local->tm_hour,
		pst_tm_local->tm_min,
		pst_tm_local->tm_sec
	);
}

/**
 * システム現在時刻を取得
 * MM/dd HH:mm:ss.sss形式
 */
static void get_sys_time_ms (char *out, size_t size)
{
	struct tm *pst_tm_local = NULL;
	struct tm st_tm_local_tmp;
	struct timespec ts;

	clock_gettime (CLOCK_REALTIME, &ts);
	pst_tm_local = localtime_r (&ts.tv_sec, &st_tm_local_tmp); /* スレッドセーフ */

	snprintf (
		out,
		size,
		"%02d/%02d %02d:%02d:%02d.%03ld",
		pst_tm_local->tm_mon+1,
		pst_tm_local->tm_mday,
		pst_tm_local->tm_hour,
		pst_tm_local->tm_min,
		pst_tm_local->tm_sec,
		ts.tv_nsec/1000000
	);
}

/**
 * pthread名称を出力
 * ログマクロ用
 */
void puts_thread_name (void)
{
	char put_name [THREAD_NAME_STRING_SIZE];
	memset (put_name, 0x00, THREAD_NAME_STRING_SIZE);
	get_thread_name (put_name, THREAD_NAME_STRING_SIZE);
	fprintf (stdout, "%s", put_name);
}

/**
 * pthread名称を取得
 */
void get_thread_name (char *out, size_t size)
{
	char name[16];
	memset (name, 0x00, sizeof(name));
	pthread_getname_np (pthread_self(), name, sizeof(name));

	strncpy (out, name, size -1);
	uint32_t len = (uint32_t)strlen(out);
	if (len < THREAD_NAME_STRING_SIZE -1) {
		int i = 0;
		for (i = 0; i < (THREAD_NAME_STRING_SIZE -1 -len); i ++) {
			*(out + len + i) = ' ';
		}
	}
}

/**
 * get_time_of_day wrapper
 * gettimeofdayはスレッドセーフ
 */
void get_time_of_day (struct timeval *p)
{
	if (!p) {
		return ;
	}

	gettimeofday (p, NULL);
}

/**
 * ログ出力用 FP切り替え
 */
void set_log_fileptr (FILE *p)
{
	if (p) {
		g_fp_log = p;
	}
}

/**
 * ログ出力用 FP取得
 */
FILE* get_log_fileptr (void)
{
	return g_fp_log;
}

void set_alternative_log (void (*_fn)(
		FILE *p_fp,
		EN_LOG_TYPE log_type,
		const char *file,
		const char *func,
		int line,
		const char *format,
		...
	)
)
{
	__puts_log = _fn;
}

/**
 * puts_log インターフェース
 * ログ出力
 */
void puts_log (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *file,
	const char *func,
	int line,
	const char *format,
	...
)
{
	if (!p_fp) {
		return;
	}

	va_list va;
	va_start (va, format); 
	puts_log_inner (p_fp, log_type, file, func, line, format, va);
	va_end (va);
}

/**
 * puts_log_inner
 * ログ出力
 */
void puts_log_inner (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *file,
	const char *func,
	int line,
	const char *format,
	va_list va
)
{
	if (!p_fp || !file || !func || !format) {
		return ;
	}

	char buf_va [LOG_STRING_SIZE];
	char time [SYSTIME_MS_STRING_SIZE];
    char thread_name [THREAD_NAME_STRING_SIZE];
	char type;
	char buff_perror[32];
	char *p_buff_perror = NULL;

	memset (buf_va, 0x00, sizeof (buf_va));
	memset (time, 0x00, sizeof (time));
    memset (thread_name, 0x00, sizeof (thread_name));
	memset (buff_perror, 0x00, sizeof (buff_perror));

	switch (log_type) {
	case EN_LOG_TYPE_D:
		type = 'D';
		break;

	case EN_LOG_TYPE_I:
		type = 'I';
		break;

	case EN_LOG_TYPE_W:
		type = 'W';
#ifndef _LOG_COLOR_OFF
		fprintf (p_fp, THM_TEXT_BOLD_TYPE);
		fprintf (p_fp, THM_TEXT_YELLOW);
#endif
		break;

	case EN_LOG_TYPE_E:
		type = 'E';
#ifndef _LOG_COLOR_OFF
		fprintf (p_fp, THM_TEXT_UNDER_LINE);
		fprintf (p_fp, THM_TEXT_BOLD_TYPE);
		fprintf (p_fp, THM_TEXT_RED);
#endif
		break;

	case EN_LOG_TYPE_PE:
		type = 'E';
#ifndef _LOG_COLOR_OFF
		fprintf (p_fp, THM_TEXT_REVERSE);
		fprintf (p_fp, THM_TEXT_BOLD_TYPE);
		fprintf (p_fp, THM_TEXT_MAGENTA);
#endif
		p_buff_perror = strerror_r(errno, buff_perror, sizeof (buff_perror));
		break;

	default:
		type = 'D';
		break;
	}

	vsnprintf (buf_va, sizeof(buf_va), format, va);

	get_sys_time_ms (time, SYSTIME_MS_STRING_SIZE);
	get_thread_name (thread_name, THREAD_NAME_STRING_SIZE);

#if 0
	delete_lF (buf_va);
	switch (log_type) {
	case EN_LOG_TYPE_PE:
		fprintf (
			p_fp,
			"[%s] %c %s  %s: %s   src=[%s %s()] line=[%d]\n",
			thread_name,
			type,
			time,
			buf_va,
			perror,
			file,
			func,
			line
		);
		break;

	case EN_LOG_TYPE_I:
	case EN_LOG_TYPE_W:
	case EN_LOG_TYPE_E:
	default:
		fprintf (
			p_fp,
			"[%s] %c %s  %s   src=[%s %s()] line=[%d]\n",
			thread_name,
			type,
			time,
			buf_va,
			file,
			func,
			line
		);
		break;
	}
	fflush (p_fp);

#else
	char *token = NULL;
	char *saveptr = NULL;
	char *s = buf_va;
	int n = 0;
	while (1) {
		token = strtok_r_impl (s, "\n", &saveptr);
		if (token == NULL) {
			if (n == 0 && (int)strlen(buf_va) > 0) {
				puts_log_fprintf (
					p_fp,
					log_type,
					thread_name,
					type,
					time,
					buf_va,
					p_buff_perror,
					file,
					func,
					line
				);
			}
			break;
		}

		puts_log_fprintf (
			p_fp,
			log_type,
			thread_name,
			type,
			time,
			token,
			p_buff_perror,
			file,
			func,
			line
		);

		s = NULL;
		++ n;
	}
#endif

#ifndef _LOG_COLOR_OFF
	fprintf (p_fp, THM_TEXT_ATTR_RESET);
#endif
	fflush (p_fp);
}

/**
 * puts_log_fprintf
 * 根っこのfpritfするところ
 */
void puts_log_fprintf (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *thread_name,
	char type,
	const char *time,
	const char *buf,
	const char *perror,
	const char *file,
	const char *func,
	int line
)
{
	if (!p_fp || !time || !buf || !file || !func) {
		return ;
	}

	switch (log_type) {
	case EN_LOG_TYPE_PE:
		fprintf (
			p_fp,
			"[%s] %c %s  %s: %s   src=[%s %s()] line=[%d]\n",
			thread_name,
			type,
			time,
			buf,
			perror,
			file,
			func,
			line
		);
		break;

	case EN_LOG_TYPE_D:
	case EN_LOG_TYPE_I:
	case EN_LOG_TYPE_W:
	case EN_LOG_TYPE_E:
	default:
		fprintf (
			p_fp,
			"[%s] %c %s  %s   src=[%s %s()] line=[%d]\n",
			thread_name,
			type,
			time,
			buf,
			file,
			func,
			line
		);
		break;
	}

	fflush (p_fp);
}

void set_alternative_log_LW (void (*_fn)(
		FILE *p_fp,
		EN_LOG_TYPE log_type,
		const char *format,
		...
	)
)
{
	__puts_log_LW = _fn;
}

/**
 * puts_log_LW インターフェース
 * ログ出力
 * (src,lineなし)
 */
void puts_log_LW (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *format,
	...
)
{
	if (!p_fp) {
		return;
	}

	va_list va;
	va_start (va, format);
	puts_log_inner_LW (p_fp, log_type, format, va);
	va_end (va);
}

/**
 * puts_log_inner_LW
 * ログ出力 src lineなし
 */
void puts_log_inner_LW (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *format,
	va_list va
)
{
	if (!p_fp || !format) {
		return ;
	}

	char buf_va [LOG_STRING_SIZE];
	char time [SYSTIME_MS_STRING_SIZE];
    char thread_name [THREAD_NAME_STRING_SIZE];
	char type;
	char buff_perror[32];
	char *p_buff_perror = NULL;

	memset (buf_va, 0x00, sizeof (buf_va));
	memset (time, 0x00, sizeof (time));
    memset (thread_name, 0x00, sizeof (thread_name));
	memset (buff_perror, 0x00, sizeof (buff_perror));

	switch (log_type) {
	case EN_LOG_TYPE_D:
		type = 'D';
		break;

	case EN_LOG_TYPE_I:
		type = 'I';
		break;

	case EN_LOG_TYPE_W:
		type = 'W';
#ifndef _LOG_COLOR_OFF
		fprintf (p_fp, THM_TEXT_BOLD_TYPE);
		fprintf (p_fp, THM_TEXT_YELLOW);
#endif
		break;

	case EN_LOG_TYPE_E:
		type = 'E';
#ifndef _LOG_COLOR_OFF
		fprintf (p_fp, THM_TEXT_UNDER_LINE);
		fprintf (p_fp, THM_TEXT_BOLD_TYPE);
		fprintf (p_fp, THM_TEXT_RED);
#endif
		break;

	case EN_LOG_TYPE_PE:
		type = 'E';
#ifndef _LOG_COLOR_OFF
		fprintf (p_fp, THM_TEXT_REVERSE);
		fprintf (p_fp, THM_TEXT_BOLD_TYPE);
		fprintf (p_fp, THM_TEXT_MAGENTA);
#endif
		p_buff_perror = strerror_r(errno, buff_perror, sizeof (buff_perror));
		break;

	default:
		type = 'D';
		break;
	}

	vsnprintf (buf_va, sizeof(buf_va), format, va);

	get_sys_time_ms (time, SYSTIME_MS_STRING_SIZE);
	get_thread_name (thread_name, THREAD_NAME_STRING_SIZE);

#if 0
	delete_lF (buf_va);
	switch (log_type) {
	case EN_LOG_TYPE_PE:
		fprintf (
			p_fp,
			"[%s] %c %s  %s: %s\n",
			thread_name,
			type,
			time,
			buf_va,
			perror
		);
		break;

	case EN_LOG_TYPE_I:
	case EN_LOG_TYPE_W:
	case EN_LOG_TYPE_E:
	default:
		fprintf (
			p_fp,
			"[%s] %c %s  %s\n",
			thread_name,
			type,
			time,
			buf_va
		);
		break;
	}
	fflush (p_fp);

#else
	char *token = NULL;
	char *saveptr = NULL;
	char *s = buf_va;
	int n = 0;
	while (1) {
		token = strtok_r_impl (s, "\n", &saveptr);
		if (token == NULL) {
			if (n == 0 && (int)strlen(buf_va) > 0) {
				puts_log_fprintf_LW (
					p_fp,
					log_type,
					thread_name,
					type,
					time,
					buf_va,
					p_buff_perror
				);
			}
			break;
		}

		puts_log_fprintf_LW (
			p_fp,
			log_type,
			thread_name,
			type,
			time,
			token,
			p_buff_perror
		);

		s = NULL;
		++ n;
	}
#endif

#ifndef _LOG_COLOR_OFF
	fprintf (p_fp, THM_TEXT_ATTR_RESET);
#endif
	fflush (p_fp);
}

/**
 * puts_log_fprintf_LW
 * 根っこのfpritfするところ
 * (src,lineなし)
 */
void puts_log_fprintf_LW (
	FILE *p_fp,
	EN_LOG_TYPE log_type,
	const char *thread_name,
	char type,
	const char *time,
	const char *buf,
	const char *perror
)
{
	if (!p_fp || !time || !buf) {
		return ;
	}

	switch (log_type) {
	case EN_LOG_TYPE_PE:
		fprintf (
			p_fp,
			"[%s] %c %s  %s: %s\n",
			thread_name,
			type,
			time,
			buf,
			perror
		);
		break;

	case EN_LOG_TYPE_D:
	case EN_LOG_TYPE_I:
	case EN_LOG_TYPE_W:
	case EN_LOG_TYPE_E:
	default:
		fprintf (
			p_fp,
			"[%s] %c %s  %s\n",
			thread_name,
			type,
			time,
			buf
		);
		break;
	}

	fflush (p_fp);
}

/**
 * delete_lF
 * 改行コードLFを削除  (文字列末尾についてるものだけです)
 * CRLFの場合 CRも削除する
 */
void delete_LF (char *p)
{
	if ((!p) || ((int)strlen(p) == 0)) {
		return;
	}

	int len = (int)strlen (p);

	while (len > 0) {
		if ((*(p + len -1) == '\n') || (*(p + len -1) == '\r')) {
			*(p + len -1) = '\0';
		} else {
			break;
		}

		-- len;
	}
}

/**
 * puts_backtrace
 *
 * libc のbacktrace
 */
extern int backtrace(void **array, int size) __attribute__ ((weak));
extern char **backtrace_symbols(void *const *array, int size) __attribute__ ((weak));
void puts_backtrace (void)
{
	int i;
	int n;
	void *p_buff [BACKTRACE_BUFF_SIZE];
	char **p_rtn;

	if (backtrace) {
		n = backtrace (p_buff, BACKTRACE_BUFF_SIZE);
		p_rtn = backtrace_symbols (p_buff, n);
		if (!p_rtn) {
			THM_PERROR ("backtrace_symbols()");
			return;
		}

		THM_LOG_W ("============================================================\n");
		THM_LOG_W ("----- pid=%d tid=%ld -----\n", getpid(), syscall(SYS_gettid));
		for (i = 0; i < n; i ++) {
			THM_LOG_W ("%s\n", p_rtn[i]);
		}
		THM_LOG_W ("============================================================\n");
		free (p_rtn);

	} else {
		THM_LOG_W ("============================================================\n");
		THM_LOG_W ("----- pid=%d tid=%ld ----- backtrace symbol not found\n", getpid(), syscall(SYS_gettid));
		THM_LOG_W ("============================================================\n");
	}
}
