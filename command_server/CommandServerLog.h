#ifndef _COMMAND_SERVER_LOG_H_
#define _COMMAND_SERVER_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Utils.h"
#include "Settings.h"


#define _COM_SVR_PRINT(fmt, ...) do {\
	fprintf (CCommandServerLog::getFileptr(), fmt, ##__VA_ARGS__);\
	fflush (CCommandServerLog::getFileptr());\
} while (0)


class CCommandServerLog
{
public:
	static void setFileptr (FILE *p) {
		if (p) {
			mp_fptr_inner = p;
		}
	}

	static FILE * getFileptr (void) {
		return mp_fptr_inner;
	}


private:
	CCommandServerLog (void) {}
	virtual ~CCommandServerLog (void) {}


	static FILE *mp_fptr_inner;

};

#endif
