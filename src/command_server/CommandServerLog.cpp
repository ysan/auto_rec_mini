#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "CommandServerLog.h"


FILE* CCommandServerLog::mp_fptr_inner = NULL;


