#ifndef _PSISI_MANAGER_STRUCTS_ADDITION_H_
#define _PSISI_MANAGER_STRUCTS_ADDITION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Utils.h"
#include "EventInformationTable_sched.h"



//---------------------------------------------------
typedef struct {
    CEventInformationTable_sched *p_parser;
} enable_parse_eit_sched_reply_param_t;



#endif
