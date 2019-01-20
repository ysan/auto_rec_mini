#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "utils.h"
#include "verstr.h"
#include "message.h"
#ifdef __cplusplus
};
#endif

#include "it9175_extern.h"


typedef struct {
	int recsec;
	char *p_destfile;
	struct timespec time_begin;
	struct OutputBuffer *pOutputBuffer;
} ST_SHARED_DATA;


static unsigned char volatile caughtSignal = 0;

static void sighandler (int arg)
{
	caughtSignal = 1;
}

static bool pre_receive (void *p_shared_data)
{
	if (!p_shared_data) {
		return false;
	}
	ST_SHARED_DATA *shared = (ST_SHARED_DATA*)p_shared_data;


	shared->pOutputBuffer = create_FileBufferedWriter( 768 * 1024, shared->p_destfile );
	if(! shared->pOutputBuffer ) {
		warn_msg(0,"failed to init FileBufferedWriter.");
		return false;
	}else{
		struct OutputBuffer * const  pFileBufferedWriter = shared->pOutputBuffer;
		shared->pOutputBuffer = create_TSParser( 8192, pFileBufferedWriter, 1);
		if(! shared->pOutputBuffer ) {
			warn_msg(0,"failed to init TS Parser.");
			OutputBuffer_release(pFileBufferedWriter);
			return false;
		}
	}

	//# time of the begin
	u_gettime(&(shared->time_begin));

	return true;
}

static void post_receive (void *p_shared_data)
{
	if (!p_shared_data) {
		return ;
	}
	ST_SHARED_DATA *shared = (ST_SHARED_DATA*)p_shared_data;


	int i = 0;
	int j = 0;
	struct timespec tmp_time_end;

	if (shared->pOutputBuffer) {
		OutputBuffer_flush (shared->pOutputBuffer);
		OutputBuffer_release (shared->pOutputBuffer);
	}

	//# time of the end
	u_gettime(&tmp_time_end);
	u_difftime(&(shared->time_begin), &tmp_time_end, &i, &j);
	msg("\n# Done! %u.%03u sec.\n", i, j);
}

static bool check_receive_loop (void *p_shared_data)
{
	if (!p_shared_data) {
		return false;
	}
	ST_SHARED_DATA *shared = (ST_SHARED_DATA*)p_shared_data;

	int i = 0;
	struct timespec tmp_time_end;

	if (caughtSignal == 1) {
		caughtSignal = 0;
		warn_msg(0,"\n# Abort!");
		return false;
	}

	if (0 < shared->recsec) {
		u_gettime(&tmp_time_end);
		u_difftime(&(shared->time_begin), &tmp_time_end, &i, NULL);
		if(i >= shared->recsec) {
			return false;
		}
	}

	return true;
}

static bool receive_from_tuner (void *p_shared_data, void *p_recv_data, int length)
{
	if (!p_shared_data) {
		return false;
	}
	ST_SHARED_DATA *shared = (ST_SHARED_DATA*)p_shared_data;


	int r = OutputBuffer_put (shared->pOutputBuffer, p_recv_data, length);
	if (r < 0) {
		warn_msg(0,"TS write failed");
	}

	return true;
}


int main(int argc, char **argv)
{
	struct Args args;

	msg("recfsusb2i " PRG_VER " " PRG_BUILT " " PRG_TIMESTAMP ", (GPL) 2015-2016 trinity19683\n");
	parseOption(argc, argv, &args);

	setSignalHandler(1, sighandler);

	bool is_log_verbose = args.flags & 0x1;


	ST_SHARED_DATA shared_data; 
	shared_data.recsec = args.recsec;
	shared_data.p_destfile = args.destfile;
	memset (&shared_data.time_begin, 0x00, sizeof(shared_data.time_begin));
	shared_data.pOutputBuffer = NULL;

	ST_IT9175_TS_RECEIVE_CALLBACKS ts_callbacks = {
		pre_receive,
		post_receive,
		check_receive_loop,
		receive_from_tuner,
		&shared_data,
	};

	it9175_extern_setup_ts_callbacks (&ts_callbacks);
	it9175_set_log_verbose (is_log_verbose);


	if (it9175_open ()) {
		it9175_tune (args.freq);
	}
	it9175_close ();


	exit (EXIT_SUCCESS);
}
