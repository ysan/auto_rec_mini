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
	EN_LOG_TYPE en_log_type,
	const char *psz_file,
	const char *psz_func,
	int n_line,
	const char *psz_format,
	...
) = puts_log;
void (*__puts_log_LW) (
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_format,
	...
) = puts_log_LW;

/*
 * Prototypes
 */
static char * strtok_r_impl (char *str, const char *delim, char **saveptr);
void puts_sys_time (void); // extern
static void get_sys_time (char *psz_out, size_t n_size);
static void get_sys_time_ms (char *psz_out, size_t n_size);
void puts_thread_name (void); // extern
void get_thread_name (char *psz_out, size_t n_size); // extern
void get_time_of_day (struct timeval *p); //extern
bool init_log (void); // extern
void init_log_stdout (void); // extern
void finaliz_log (void); // extern
void set_log_fileptr (FILE *p); // extern
FILE* get_log_fileptr (void);
void set_alternative_log (void (*_fn)( // extern
		FILE *p_fp,
		EN_LOG_TYPE en_log_type,
		const char *psz_file,
		const char *psz_func,
		int n_line,
		const char *psz_format,
		...
	)
);
void puts_log ( // extern
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_file,
	const char *psz_func,
	int n_line,
	const char *psz_format,
	...
);
static void puts_log_inner (
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_file,
	const char *psz_func,
	int n_line,
	const char *psz_format,
	va_list va
);
static void puts_log_fprintf (
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_thread_name,
	char type,
	const char *psz_time,
	const char *psz_buf,
	const char *psz_perror,
	const char *psz_file,
	const char *psz_func,
	int n_line
);
void set_alternative_log_LW (void (*_fn)( // extern
		FILE *p_fp,
		EN_LOG_TYPE en_log_type,
		const char *psz_format,
		...
	)
);
void puts_log_LW ( // extern
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_format,
	...
);
static void puts_log_inner_LW (
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_format,
	va_list va
);
static void puts_log_fprintf_LW (
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_thread_name,
	char type,
	const char *psz_time,
	const char *psz_buf,
	const char *psz_perror
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
	char sz_time [SYSTIME_STRING_SIZE];
	memset (sz_time, 0x00, SYSTIME_STRING_SIZE);
	get_sys_time (sz_time, sizeof (sz_time));
	fprintf (stdout, "%s", sz_time);
}

/**
 * システム現在時刻を取得
 * MM/dd HH:mm:ss形式
 */
static void get_sys_time (char *psz_out, size_t n_size)
{
	time_t timer;
	struct tm *pst_tm_local = NULL;
	struct tm st_tm_local_tmp;

	timer = time (NULL);
	pst_tm_local = localtime_r (&timer, &st_tm_local_tmp); /* スレッドセーフ */

	snprintf (
		psz_out,
		n_size,
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
static void get_sys_time_ms (char *psz_out, size_t n_size)
{
	struct tm *pst_tm_local = NULL;
	struct tm st_tm_local_tmp;
	struct timespec ts;

	clock_gettime (CLOCK_REALTIME, &ts);
	pst_tm_local = localtime_r (&ts.tv_sec, &st_tm_local_tmp); /* スレッドセーフ */

	snprintf (
		psz_out,
		n_size,
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
	char sz_put_name [THREAD_NAME_STRING_SIZE];
	memset (sz_put_name, 0x00, THREAD_NAME_STRING_SIZE);
	get_thread_name (sz_put_name, THREAD_NAME_STRING_SIZE);
	fprintf (stdout, "%s", sz_put_name);
}

/**
 * pthread名称を取得
 */
void get_thread_name (char *psz_out, size_t n_size)
{
	char sz_name[16];
	memset (sz_name, 0x00, sizeof(sz_name));
	pthread_getname_np (pthread_self(), sz_name, sizeof(sz_name));

	strncpy (psz_out, sz_name, n_size -1);
	uint32_t len = (uint32_t)strlen(psz_out);
	if (len < THREAD_NAME_STRING_SIZE -1) {
		int i = 0;
		for (i = 0; i < (THREAD_NAME_STRING_SIZE -1 -len); i ++) {
			*(psz_out + len + i) = ' ';
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
		EN_LOG_TYPE en_log_type,
		const char *psz_file,
		const char *psz_func,
		int n_line,
		const char *psz_format,
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
	EN_LOG_TYPE en_log_type,
	const char *psz_file,
	const char *psz_func,
	int n_line,
	const char *psz_format,
	...
)
{
	if (!p_fp) {
		return;
	}

	va_list va;
	va_start (va, psz_format); 
	puts_log_inner (p_fp, en_log_type, psz_file, psz_func, n_line, psz_format, va);
	va_end (va);
}

/**
 * puts_log_inner
 * ログ出力
 */
void puts_log_inner (
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_file,
	const char *psz_func,
	int n_line,
	const char *psz_format,
	va_list va
)
{
	if (!p_fp || !psz_file || !psz_func || !psz_format) {
		return ;
	}

	char sz_buf_va [LOG_STRING_SIZE];
	char sz_time [SYSTIME_MS_STRING_SIZE];
    char sz_thread_name [THREAD_NAME_STRING_SIZE];
	char type;
	char sz_perror[32];
	char *psz_perror = NULL;

	memset (sz_buf_va, 0x00, sizeof (sz_buf_va));
	memset (sz_time, 0x00, sizeof (sz_time));
    memset (sz_thread_name, 0x00, sizeof (sz_thread_name));
	memset (sz_perror, 0x00, sizeof (sz_perror));

	switch (en_log_type) {
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
		psz_perror = strerror_r(errno, sz_perror, sizeof (sz_perror));
		break;

	default:
		type = 'D';
		break;
	}

	vsnprintf (sz_buf_va, sizeof(sz_buf_va), psz_format, va);

	get_sys_time_ms (sz_time, SYSTIME_MS_STRING_SIZE);
	get_thread_name (sz_thread_name, THREAD_NAME_STRING_SIZE);

#if 0
	delete_lF (sz_buf_va);
	switch (en_log_type) {
	case EN_LOG_TYPE_PE:
		fprintf (
			p_fp,
			"[%s] %c %s  %s: %s   src=[%s %s()] line=[%d]\n",
			sz_thread_name,
			type,
			sz_time,
			sz_buf_va,
			psz_perror,
			psz_file,
			psz_func,
			n_line
		);
		break;

	case EN_LOG_TYPE_I:
	case EN_LOG_TYPE_W:
	case EN_LOG_TYPE_E:
	default:
		fprintf (
			p_fp,
			"[%s] %c %s  %s   src=[%s %s()] line=[%d]\n",
			sz_thread_name,
			type,
			sz_time,
			sz_buf_va,
			psz_file,
			psz_func,
			n_line
		);
		break;
	}
	fflush (p_fp);

#else
	char *token = NULL;
	char *saveptr = NULL;
	char *s = sz_buf_va;
	int n = 0;
	while (1) {
		token = strtok_r_impl (s, "\n", &saveptr);
		if (token == NULL) {
			if (n == 0 && (int)strlen(sz_buf_va) > 0) {
				puts_log_fprintf (
					p_fp,
					en_log_type,
					sz_thread_name,
					type,
					sz_time,
					sz_buf_va,
					psz_perror,
					psz_file,
					psz_func,
					n_line
				);
			}
			break;
		}

		puts_log_fprintf (
			p_fp,
			en_log_type,
			sz_thread_name,
			type,
			sz_time,
			token,
			psz_perror,
			psz_file,
			psz_func,
			n_line
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
	EN_LOG_TYPE en_log_type,
	const char *psz_thread_name,
	char type,
	const char *psz_time,
	const char *psz_buf,
	const char *psz_perror,
	const char *psz_file,
	const char *psz_func,
	int n_line
)
{
	if (!p_fp || !psz_time || !psz_buf || !psz_file || !psz_func) {
		return ;
	}

	switch (en_log_type) {
	case EN_LOG_TYPE_PE:
		fprintf (
			p_fp,
			"[%s] %c %s  %s: %s   src=[%s %s()] line=[%d]\n",
			psz_thread_name,
			type,
			psz_time,
			psz_buf,
			psz_perror,
			psz_file,
			psz_func,
			n_line
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
			psz_thread_name,
			type,
			psz_time,
			psz_buf,
			psz_file,
			psz_func,
			n_line
		);
		break;
	}

	fflush (p_fp);
}

void set_alternative_log_LW (void (*_fn)(
		FILE *p_fp,
		EN_LOG_TYPE en_log_type,
		const char *psz_format,
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
	EN_LOG_TYPE en_log_type,
	const char *psz_format,
	...
)
{
	if (!p_fp) {
		return;
	}

	va_list va;
	va_start (va, psz_format);
	puts_log_inner_LW (p_fp, en_log_type, psz_format, va);
	va_end (va);
}

/**
 * puts_log_inner_LW
 * ログ出力 src lineなし
 */
void puts_log_inner_LW (
	FILE *p_fp,
	EN_LOG_TYPE en_log_type,
	const char *psz_format,
	va_list va
)
{
	if (!p_fp || !psz_format) {
		return ;
	}

	char sz_buf_va [LOG_STRING_SIZE];
	char sz_time [SYSTIME_MS_STRING_SIZE];
    char sz_thread_name [THREAD_NAME_STRING_SIZE];
	char type;
	char sz_perror[32];
	char *psz_perror = NULL;

	memset (sz_buf_va, 0x00, sizeof (sz_buf_va));
	memset (sz_time, 0x00, sizeof (sz_time));
    memset (sz_thread_name, 0x00, sizeof (sz_thread_name));
	memset (sz_perror, 0x00, sizeof (sz_perror));

	switch (en_log_type) {
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
		psz_perror = strerror_r(errno, sz_perror, sizeof (sz_perror));
		break;

	default:
		type = 'D';
		break;
	}

	vsnprintf (sz_buf_va, sizeof(sz_buf_va), psz_format, va);

	get_sys_time_ms (sz_time, SYSTIME_MS_STRING_SIZE);
	get_thread_name (sz_thread_name, THREAD_NAME_STRING_SIZE);

#if 0
	delete_lF (sz_buf_va);
	switch (en_log_type) {
	case EN_LOG_TYPE_PE:
		fprintf (
			p_fp,
			"[%s] %c %s  %s: %s\n",
			sz_thread_name,
			type,
			sz_time,
			sz_buf_va,
			psz_perror
		);
		break;

	case EN_LOG_TYPE_I:
	case EN_LOG_TYPE_W:
	case EN_LOG_TYPE_E:
	default:
		fprintf (
			p_fp,
			"[%s] %c %s  %s\n",
			sz_thread_name,
			type,
			sz_time,
			sz_buf_va
		);
		break;
	}
	fflush (p_fp);

#else
	char *token = NULL;
	char *saveptr = NULL;
	char *s = sz_buf_va;
	int n = 0;
	while (1) {
		token = strtok_r_impl (s, "\n", &saveptr);
		if (token == NULL) {
			if (n == 0 && (int)strlen(sz_buf_va) > 0) {
				puts_log_fprintf_LW (
					p_fp,
					en_log_type,
					sz_thread_name,
					type,
					sz_time,
					sz_buf_va,
					psz_perror
				);
			}
			break;
		}

		puts_log_fprintf_LW (
			p_fp,
			en_log_type,
			sz_thread_name,
			type,
			sz_time,
			token,
			psz_perror
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
	EN_LOG_TYPE en_log_type,
	const char *psz_thread_name,
	char type,
	const char *psz_time,
	const char *psz_buf,
	const char *psz_perror
)
{
	if (!p_fp || !psz_time || !psz_buf) {
		return ;
	}

	switch (en_log_type) {
	case EN_LOG_TYPE_PE:
		fprintf (
			p_fp,
			"[%s] %c %s  %s: %s\n",
			psz_thread_name,
			type,
			psz_time,
			psz_buf,
			psz_perror
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
			psz_thread_name,
			type,
			psz_time,
			psz_buf
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
