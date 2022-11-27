#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include "Utils.h"


#define THREAD_NAME_STRING_SIZE		(10+1)
#define BACKTRACE_BUFF_SIZE			(20)


#ifdef _ANDROID_BUILD
#include <stdint.h>
#include <unwind.h>
#include <cxxabi.h>

struct UnwindData {
	void **array;
	int current;
	int size;
};

static _Unwind_Reason_Code unwind_callback (struct _Unwind_Context* context, void* _data)
{
	UnwindData *data = static_cast<UnwindData*> (_data);
	uintptr_t pc = _Unwind_GetIP (context);
	if (pc) {
		if (data->current < data->size) {
			data->array[data->current] = (void *)pc;
			++ data->current;
		} else {
			return _URC_END_OF_STACK;
		}
	}

	return _URC_NO_REASON;
}

int bionic_backtrace (void **array, int size)
{
	UnwindData data = {array, 0, size};
	_Unwind_Backtrace (unwind_callback, &data);

	return data.current;
}

char **bionic_backtrace_symbols (void *const *array, int size)
{
	const int line_size = 256;
	void *buff = malloc((sizeof(void *) * size) + (line_size * size));
	char **line_addr = (char **)buff;
	char *ptr = (char *)buff + sizeof(void *) * size;

	for (int i = 0; i < size; ++ i) {
		line_addr[i] = ptr;
		const void *addr = array[i];
		const char *symbol = "";
		const char *fname  = "";
		unsigned int offset = 0;

		Dl_info info;
		if (dladdr(addr, &info) && info.dli_sname) {
			int status = 0;
			symbol = info.dli_sname;
			char *demangled = __cxxabiv1::__cxa_demangle (symbol, 0, 0, &status);
			if (demangled) {
				symbol = demangled;
			}
			fname = info.dli_fname;
			offset = (unsigned int)addr;
			offset -= (unsigned int)info.dli_saddr;
			snprintf (line_addr[i], line_size, "[0x%08X] %s <%s +0x%X>", (unsigned int)addr, fname, symbol, (unsigned int)offset);
			if (demangled) {
				free (demangled);
			}
		} else {
			snprintf (line_addr[i], line_size, "[0x%08X]", (unsigned int)addr);
		}
		ptr += line_size;
	}

	return line_addr;
}

#endif


char * strtok_r_impl (char *str, const char *delim, char **saveptr, bool *is_tail_delim)
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

		if (is_tail_delim) {
			if ((int)strlen(f) == (int)strlen(delim)) {
				*is_tail_delim = true;
			}
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

		if (is_tail_delim) {
			if ((int)strlen(f) == (int)strlen(delim)) {
				*is_tail_delim = true;
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
 * DISKの空き容量 MByte単位で返します
 */
int CUtils::getDiskFreeMB (const char *path)
{
	if (!path) {
		return -1;
	}

	struct statvfs s = {0};
	int r = statvfs (path, &s);
	if (r != 0) {
		_UTL_PERROR ("statvfs");
		return -1;
	}

	long long _free = (long long)s.f_bfree * (long long)s.f_bsize / 1000000;
	return (int) _free;
}

/**
 * readFile
 * fdを読みnSize byteのデータをpBuffに返す
 */
int CUtils::readFile (int fd, uint8_t *pBuff, size_t nSize)
{
	if ((!pBuff) || (nSize == 0)) {
		return -1;
	}

	int nReadSize = 0;
	int nDone = 0;

	while (1) {
		nReadSize = read (fd, pBuff, nSize);
		if (nReadSize < 0) {
			perror ("read()");
			return -1;

		} else if (nReadSize == 0) {
			// file end
			break;

		} else {
			// read done
			pBuff += nReadSize;
			nSize -= nReadSize;
			nDone += nReadSize;

			if (nSize == 0) {
				break;
			}
		}
	}

	return nDone;
}

/**
 * recvData
 * fdを読みnSize byteのデータをpBuffに返す
 */
int CUtils::recvData (int fd, uint8_t *pBuff, int buffSize, bool *p_isDisconnect)
{
	if (!pBuff || buffSize <= 0) {
		return -1;
	}

	int rSize = 0;
	int rSizeTotal = 0;
	struct timeval tv;
	fd_set fds;

	while (1) {
		if (buffSize == 0) {
			// check buffer size
			printf ("buffer over.\n");
			return -2;
		}

		rSize = recv (fd, pBuff, buffSize, 0);
		if (rSize < 0) {
			perror ("recv()");
			return -1;

		} else if (rSize == 0) {
			// disconnect
			if (p_isDisconnect) {
				*p_isDisconnect = true;
			}
			break;

		} else {
			// recv ok
			pBuff += rSize;
			buffSize -= rSize;
			rSizeTotal += rSize;
//CUtils::dumper (pBuff-rSize, rSize);

			//------------ 接続先からの受信が終了したか ------------
			FD_ZERO (&fds);
			FD_SET (fd, &fds);
			tv.tv_sec = 0;
			tv.tv_usec = 0; // timeout not use
			int r = select (fd+1, &fds, NULL, NULL, &tv);
			if (r < 0) {
				perror ("select()");
				return -3;
			} else if (r == 0) {
				// timeout not use
			}

			// レディではなくなったらループから抜ける
			if (!FD_ISSET (fd, &fds)) {
				break;
			}
			//------------------------------------------------------
		}
	}

	return rSizeTotal;
}

/**
 * deleteLF
 * 改行コードLFを削除  (文字列末尾についてるものだけです)
 * CRLFの場合 CRも削除する
 */
void CUtils::deleteLF (char *p)
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
 * deleteHeadSp
 * 文字列先頭のスペース削除
 */
void CUtils::deleteHeadSp (char *p)
{
	if ((!p) || ((int)strlen(p) == 0)) {
		return;
	}

	int i = 0;

	while (1) {
		if (*p == ' ') {
			if ((int)strlen(p) > 1) {
				i = 0;
				while (*(p+i)) {
					*(p+i) = *(p+i+1);
					++ i;
				}

			} else {
				// 全文字スペースだった
				*p = 0x00;
			}
		} else {
			break;
		}
	}
}

/**
 * deleteTailSp
 * 文字列末尾のスペース削除
 */
void CUtils::deleteTailSp (char *p)
{
	if (!p) {
		return;
	}

	int nLen = (int)strlen(p);
	if (nLen == 0) {
		return;
	}

	-- nLen;
	while (nLen >= 0) {
		if (*(p+nLen) == ' ') {
			*(p+nLen) = 0x00;
		} else {
			break;
		}
		-- nLen;
	}
}

/**
 * putsBackTrace
 * libc とbionicはコンパイルスイッチで切り替え
 */
void CUtils::putsBackTrace (void)
{
	int i;
	int n;
	void *pBuff [BACKTRACE_BUFF_SIZE];
	char **pRtn;

#ifdef _ANDROID_BUILD
	n = bionic_backtrace (pBuff, BACKTRACE_BUFF_SIZE);
	pRtn = bionic_backtrace_symbols (pBuff, n);
#else
	if (!backtrace) {
		_UTL_LOG_W ("============================================================\n");
		_UTL_LOG_W ("----- pid=%d tid=%ld ----- backtrace symbol not found\n", getpid(), syscall(SYS_gettid));
		_UTL_LOG_W ("============================================================\n");
		return ;
	}

	// libc
	n = backtrace (pBuff, BACKTRACE_BUFF_SIZE);
	pRtn = backtrace_symbols (pBuff, n);
#endif
	if (!pRtn) {
		_UTL_PERROR ("backtrace_symbols()");
		return;
	}

	_UTL_LOG_W ("============================================================\n");
	_UTL_LOG_W ("----- pid=%d tid=%ld -----\n", getpid(), syscall(SYS_gettid));
	for (i = 0; i < n; ++ i) {
		_UTL_LOG_W ("%s\n", pRtn[i]);
	}
	_UTL_LOG_W ("============================================================\n");
	free (pRtn);
}

#define DUMP_PUTS_OFFSET	"  "
void CUtils::dumper (const uint8_t *pSrc, int nSrcLen, bool isAddAscii)
{
	if ((!pSrc) || (nSrcLen <= 0)) {
		return;
	}

	int i = 0;
	int j = 0;
	int k = 0;

	char szWk [128];
	char* pszWk = &szWk[0];
	memset (szWk, 0x00, sizeof(szWk));
	int rtn = 0;
	int size = sizeof(szWk);


	while (nSrcLen >= 16) {
		// clear for snprintf
		pszWk = &szWk[0];
		memset (szWk, 0x00, sizeof(szWk));
		size = sizeof(szWk);


//		fprintf (stdout, "%s0x%08x: ", DUMP_PUTS_OFFSET, i);
		rtn = snprintf (pszWk, size, "%s0x%08x: ", DUMP_PUTS_OFFSET, i);
		pszWk += rtn;
		size -= rtn;

		// 16進dump
//		fprintf (
//			stdout,
//			"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x",
//			*(pSrc+ 0), *(pSrc+ 1), *(pSrc+ 2), *(pSrc+ 3), *(pSrc+ 4), *(pSrc+ 5), *(pSrc+ 6), *(pSrc+ 7),
//			*(pSrc+ 8), *(pSrc+ 9), *(pSrc+10), *(pSrc+11), *(pSrc+12), *(pSrc+13), *(pSrc+14), *(pSrc+15)
//		);
		rtn = snprintf (
			pszWk,
			size,
			"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x",
			*(pSrc+ 0), *(pSrc+ 1), *(pSrc+ 2), *(pSrc+ 3), *(pSrc+ 4), *(pSrc+ 5), *(pSrc+ 6), *(pSrc+ 7),
			*(pSrc+ 8), *(pSrc+ 9), *(pSrc+10), *(pSrc+11), *(pSrc+12), *(pSrc+13), *(pSrc+14), *(pSrc+15)
		);
		pszWk += rtn;
		size -= rtn;

		// ascii文字表示
		if (isAddAscii) {
//			fprintf (stdout, "  |");
			rtn = snprintf (pszWk, size, "  |");
			pszWk += rtn;
			size -= rtn;

			k = 0;
			while (k < 16) {
				// 制御コード系は'.'で代替
//				fprintf (
//					stdout,
//					"%c",
//					(*(pSrc+k)>0x1f) && (*(pSrc+k)<0x7f) ? *(pSrc+k) : '.'
//				);
				rtn = snprintf (
					pszWk,
					size,
					"%c",
					(*(pSrc+k)>0x1f) && (*(pSrc+k)<0x7f) ? *(pSrc+k) : '.'
				);
				pszWk += rtn;
				size -= rtn;

				++ k;
			}
		}

//		fprintf (stdout, "|\n");
		rtn = snprintf (pszWk, size, "|\n");
		pszWk += rtn;
		size -= rtn;

		_UTL_LOG_I ("%s", szWk);

		pSrc += 16;
		i += 16;
		nSrcLen -= 16;
	}

	// clear for snprintf
	pszWk = &szWk[0];
	memset (szWk, 0x00, sizeof(szWk));
	size = sizeof(szWk);

	// 余り分(16byte満たない分)
	if (nSrcLen) {

		// 16進dump
//		fprintf (stdout, "%s0x%08x: ", DUMP_PUTS_OFFSET, i);
		rtn = snprintf (pszWk, size, "%s0x%08x: ", DUMP_PUTS_OFFSET, i);
		pszWk += rtn;
		size -= rtn;

		while (j < 16) {
			if (j < nSrcLen) {
//				fprintf (stdout, "%02x", *(pSrc+j));
				rtn = snprintf (pszWk, size, "%02x", *(pSrc+j));
				pszWk += rtn;
				size -= rtn;

				if (j == 7) {
//					fprintf (stdout, "  ");
					rtn = snprintf (pszWk, size, "  ");
					pszWk += rtn;
					size -= rtn;
				} else if (j == 15) {

				} else {
//					fprintf (stdout, " ");
					rtn = snprintf (pszWk, size, " ");
					pszWk += rtn;
					size -= rtn;
				}

			} else {
//				fprintf (stdout, "  ");
				rtn = snprintf (pszWk, size, "  ");
				pszWk += rtn;
				size -= rtn;

				if (j == 7) {
//					fprintf (stdout, "  ");
					rtn = snprintf (pszWk, size, "  ");
					pszWk += rtn;
					size -= rtn;
				} else if (j == 15) {

				} else {
//					fprintf (stdout, " ");
					rtn = snprintf (pszWk, size, " ");
					pszWk += rtn;
					size -= rtn;
				}
			}

			++ j;
		}

		// ascii文字表示
		if (isAddAscii) {
//			fprintf (stdout, "  |");
			rtn = snprintf (pszWk, size, "  |");
			pszWk += rtn;
			size -= rtn;

			k = 0;
			while (k < nSrcLen) {
				// 制御コード系は'.'で代替
//				fprintf (stdout, "%c", (*(pSrc+k)>0x20) && (*(pSrc+k)<0x7f) ? *(pSrc+k) : '.');
				rtn = snprintf (pszWk, size, "%c", (*(pSrc+k)>0x20) && (*(pSrc+k)<0x7f) ? *(pSrc+k) : '.');
				pszWk += rtn;
				size -= rtn;

				++ k;
			}
			for (int i = 0; i < (16 - nSrcLen); ++ i) {
//				fprintf (stdout, " ");
				rtn = snprintf (pszWk, size, " ");
				pszWk += rtn;
				size -= rtn;
			}
		}

//		fprintf (stdout, "|\n");
		rtn = snprintf (pszWk, size, "|\n");
		pszWk += rtn;
		size -= rtn;

		_UTL_LOG_I ("%s", szWk);
	}
}

#if 0
#define DUMP_PUTS_OFFSET	"  "
void CUtils::dumper (const uint8_t *pSrc, int nSrcLen, bool isAddAscii)
{
	if ((!pSrc) || (nSrcLen <= 0)) {
		return;
	}

	int i = 0;
	int j = 0;
	int k = 0;

	while (nSrcLen >= 16) {

		fprintf (stdout, "%s0x%08x: ", DUMP_PUTS_OFFSET, i);

		// 16進dump
		fprintf (
			stdout,
			"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x",
			*(pSrc+ 0), *(pSrc+ 1), *(pSrc+ 2), *(pSrc+ 3), *(pSrc+ 4), *(pSrc+ 5), *(pSrc+ 6), *(pSrc+ 7),
			*(pSrc+ 8), *(pSrc+ 9), *(pSrc+10), *(pSrc+11), *(pSrc+12), *(pSrc+13), *(pSrc+14), *(pSrc+15)
		);

		// ascii文字表示
		if (isAddAscii) {
			fprintf (stdout, "  |");
			k = 0;
			while (k < 16) {
				// 制御コード系は'.'で代替
				fprintf (
					stdout,
					"%c",
					(*(pSrc+k)>0x1f) && (*(pSrc+k)<0x7f) ? *(pSrc+k) : '.'
				);
				++ k;
			}
		}

		fprintf (stdout, "|\n");

		pSrc += 16;
		i += 16;
		nSrcLen -= 16;
	}

	// 余り分(16byte満たない分)
	if (nSrcLen) {

		// 16進dump
		fprintf (stdout, "%s0x%08x: ", DUMP_PUTS_OFFSET, i);
		while (j < 16) {
			if (j < nSrcLen) {
				fprintf (stdout, "%02x", *(pSrc+j));
				if (j == 7) {
					fprintf (stdout, "  ");
				} else if (j == 15) {

				} else {
					fprintf (stdout, " ");
				}

			} else {
				fprintf (stdout, "  ");
				if (j == 7) {
					fprintf (stdout, "  ");
				} else if (j == 15) {

				} else {
					fprintf (stdout, " ");
				}
			}

			++ j;
		}

		// ascii文字表示
		if (isAddAscii) {
			fprintf (stdout, "  |");
			k = 0;
			while (k < nSrcLen) {
				// 制御コード系は'.'で代替
				fprintf (stdout, "%c", (*(pSrc+k)>0x20) && (*(pSrc+k)<0x7f) ? *(pSrc+k) : '.');
				++ k;
			}
			for (int i = 0; i < (16 - nSrcLen); ++ i) {
				fprintf (stdout, " ");
			}
		}

		fprintf (stdout, "|\n");
	}
}
#endif

std::vector<std::string> CUtils::split (std::string s, char delim, bool ignore_empty)
{
	int first = 0;
	int last = s.find_first_of (delim);
				 
	std::vector <std::string> r;
	while (first < s.size()) {
		if (ignore_empty && ((first - last) == 0)) {
			;;
		} else {
			std::string sub (s, first, last - first);
			r.push_back (sub);
		}
		first = last + 1;
		last = s.find_first_of (delim, first);
		if (last == std::string::npos) {
			last = s.size();
		}
	}

	return r;
}

CLogger *CUtils::m_logger = NULL;

void CUtils::set_logger (CLogger *logger)
{
	m_logger = logger;
}

CLogger* CUtils::get_logger (void)
{
	return m_logger;
}

CShared<> *CUtils::m_shared = NULL;

void CUtils::set_shared (CShared<> *shared)
{
	m_shared = shared;
}

CShared<>* CUtils::get_shared (void)
{
	return m_shared;
}
