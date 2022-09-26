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
	if (CUtils::get_logger()) { \
		CUtils::get_logger()->puts_without_header(fmt, ##__VA_ARGS__); \
	} \
} while (0)

#endif
