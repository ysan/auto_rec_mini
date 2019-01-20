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
	bool (*pcb_pre_ts_receive) (void *p_shared_data);
	void (*pcb_post_ts_receive) (void *p_shared_data);
	bool (*pcb_check_ts_receive_loop) (void *p_shared_data);
	bool (*pcb_ts_received) (void *p_shared_data, void *p_ts_data, int length);
	void *p_shared_data;
} ST_IT9175_TS_RECEIVE_CALLBACKS;


#ifdef __cplusplus
extern "C" {
#endif

//
// external
//
extern void it9175_extern_setup_ts_callbacks (const ST_IT9175_TS_RECEIVE_CALLBACKS *p_callbacks);
extern EN_IT9175_STATE it9175_get_state (void);
extern void it9175_set_log_verbose (bool is_log_verbose);
extern bool it9175_open (void);
extern void it9175_close (void);
extern int it9175_tune (unsigned int freq);
extern void it9175_force_tune_end (void);

#ifdef __cplusplus
};
#endif
