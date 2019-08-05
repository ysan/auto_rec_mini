/* fsusb2i   (c) 2015-2016 trinity19683
  message (Linux OSes)
  message.h
  2016-01-02
*/
#pragma once

#include "it9175_extern.h"

#ifdef __cplusplus
extern "C" {
#endif
void u_debugMessage(const unsigned int flags, const char* FuncName, const unsigned int Line, const int retCode, const char* fmt, ...);
void u_debugMessage_syslog(const unsigned int flags, const char* FuncName, const unsigned int Line, const int retCode, const char* fmt, ...);
void dumpHex(char* const buf, const unsigned buflen, const int addr, const void* const dptr, unsigned dsize);
#ifdef __cplusplus
}
#endif

#define msg(...)  fprintf(it9175_getLogFileptr(),__VA_ARGS__)
#define warn_msg(errCode, ...) do {\
	u_debugMessage(1, 0, 0, errCode, __VA_ARGS__);\
	if (it9175_isUseSyslog()) {\
		u_debugMessage_syslog(1, 0, 0, errCode, __VA_ARGS__);\
	}\
} while (0)
#define warn_info(errCode, ...) do {\
	u_debugMessage(1, __func__, __LINE__, errCode, __VA_ARGS__);\
	if (it9175_isUseSyslog()) {\
		u_debugMessage_syslog(1, __func__, __LINE__, errCode, __VA_ARGS__);\
	}\
} while (0)

#ifdef DEBUG
#define dmsg(...) do {\
	u_debugMessage(1, 0, 0, 0, __VA_ARGS__);\
	if (it9175_isUseSyslog()) {\
		u_debugMessage_syslog(1, 0, 0, 0, __VA_ARGS__);\
	}\
} while (0)
#define dmsgn(...) do {\
	u_debugMessage(0, 0, 0, 0, __VA_ARGS__);\
	if (it9175_isUseSyslog()) {\
		u_debugMessage_syslog(0, 0, 0, 0, __VA_ARGS__);\
	}\
} while (0)
#else
#define dmsg(...)
#define dmsgn(...)
#endif

/*EOF*/
