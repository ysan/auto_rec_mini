#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>


//
// type defines
//
typedef enum {
	EN_IT9175_STATE__CLOSED = 0,
	EN_IT9175_STATE__OPENED,
	EN_IT9175_STATE__TUNE_BEGINED,
	EN_IT9175_STATE__TUNED,
	EN_IT9175_STATE__TUNE_ENDED,
} EN_IT9175_STATE;

typedef struct {
	bool (*pcb_pre_receive) (void);
	void (*pcb_post_receive) (void);
	bool (*pcb_check_receive_loop) (void);
	bool (*pcb_receive_from_tuner) (void *p_recv_data, int length);
	void *p_shared_data;
} ST_IT9175_SETUP_INFO;


#ifdef __cplusplus
extern "C" {
#endif

//
// external
//
extern void it9175_extern_setup (ST_IT9175_SETUP_INFO *p_info);
extern EN_IT9175_STATE it9175_get_state (void);
extern void it9175_set_log_verbose (bool is_log_verbose);
extern bool it9175_open (void);
extern void it9175_close (void);
extern int it9175_tune (unsigned int freq);

#ifdef __cplusplus
};
#endif
