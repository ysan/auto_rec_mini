/* fsusb2i   (c) 2015 trinity19683
  message (Linux OSes)
  message.c
  2015-12-30
*/

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include "message.h"
#include "it9175_extern.h"

void u_debugMessage(const unsigned int flags, const char* FuncName, const unsigned int Line, const int retCode, const char* fmt, ...)
{
	va_list ap;

	if(0 != FuncName) {
		fprintf(it9175_getLogFileptr(), "@%s ", FuncName);
	}
	if(0 != Line) {
		fprintf(it9175_getLogFileptr(), "L%u ", Line);
	}

	va_start(ap, fmt);
	vfprintf(it9175_getLogFileptr(), fmt, ap);
	va_end(ap);

	if(0 != retCode) {
		fprintf(it9175_getLogFileptr(), ", ERR=%d", retCode);
	}
	if(flags & 0x1U) {
		fprintf(it9175_getLogFileptr(), "\n");
	}
}

void u_debugMessage_syslog(const unsigned int flags, const char* FuncName, const unsigned int Line, const int retCode, const char* fmt, ...)
{
	char buf [1024] = {0};
	int n = 0;
	va_list ap;

	int buf_size = sizeof(buf);
	char *pw = buf;

	if(0 != FuncName) {
		n = snprintf(buf + n, buf_size, "@%s ", FuncName);
		pw += n;
		buf_size -= n;
		
	}
	if(0 != Line) {
		n = snprintf(buf + n, buf_size, "L%u ", Line);
		pw += n;
		buf_size -= n;
	}

	va_start(ap, fmt);
	n += vsnprintf(buf + n, buf_size, fmt, ap);
	va_end(ap);

	if(0 != retCode) {
		n = snprintf(buf + n, buf_size, ", ERR=%d", retCode);
		pw += n;
		buf_size -= n;
	}
	if(flags & 0x1U) {
		n = snprintf(buf + n, buf_size, "\n");
		pw += n;
		buf_size -= n;
	}


	syslog (LOG_INFO, "%s", buf);

}

void dumpHex(char* const buf, const unsigned buflen, const int addr, const void* const dptr, unsigned dsize)
{
	int idx = 0, ret;
	const unsigned char* ptr = dptr;
	if(0 <= addr) {
		ret = sprintf(buf, "%04X: ", addr);
		idx += ret;
	}
	if(dsize * 3 > buflen - idx) {
		warn_info(0,"buffer overflow");
		return;
	}
	while(dsize) {
		ret = sprintf(&buf[idx], "%02X", *ptr);
		idx += ret;
		ptr++;
		dsize--;
		if(dsize) {
			buf[idx] = ' ';
			idx++;
		}
	}
	buf[idx] = 0;
}

/*EOF*/
