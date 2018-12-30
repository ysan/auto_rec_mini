/* fsusb2i   (c) 2015-2016 trinity19683
  recfsusb2i program (Linux OSes)
  main.c
  2016-02-12
  License: GNU GPLv3
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#include "usbdevfile.h"
#include "it9175.h"
#include "tsthread.h"
#include "utils.h"

#include "verstr.h"
#include "message.h"

#include "it9175_extern.h"


static unsigned char volatile  caughtSignal = 0;
static void sighandler(int arg)    { caughtSignal = 1; }

int g_recsec = 0;
char * g_destfile = NULL;
struct timespec g_time_begin;
struct OutputBuffer *g_pOutputBuffer = NULL;

static bool pre_receive (void)
{
	g_pOutputBuffer = create_FileBufferedWriter( 768 * 1024, g_destfile );
	if(! g_pOutputBuffer ) {
		warn_msg(0,"failed to init FileBufferedWriter.");
		return false;
	}else{
		struct OutputBuffer * const  pFileBufferedWriter = g_pOutputBuffer;
		g_pOutputBuffer = create_TSParser( 8192, pFileBufferedWriter, 1);
		if(! g_pOutputBuffer ) {
			warn_msg(0,"failed to init TS Parser.");
			OutputBuffer_release(pFileBufferedWriter);
			return false;
		}
	}

	//# time of the begin
	u_gettime(&g_time_begin);

	return true;
}

static void post_receive (void)
{
	int i = 0;
	int j = 0;
	struct timespec tmp_time_end;

	if (g_pOutputBuffer) {
		OutputBuffer_flush (g_pOutputBuffer);
		OutputBuffer_release (g_pOutputBuffer);
	}

	//# time of the end
	u_gettime(&tmp_time_end);
	u_difftime(&g_time_begin, &tmp_time_end, &i, &j);
	msg("\n# Done! %u.%03u sec.\n", i, j);
}

static bool check_receive_loop (void)
{
	int i = 0;
	struct timespec tmp_time_end;

	if (caughtSignal == 1) {
		caughtSignal = 0;
		warn_msg(0,"\n# Abort!");
		return false;
	}

	if (0 < g_recsec) {
		u_gettime(&tmp_time_end);
		u_difftime(&g_time_begin, &tmp_time_end, &i, NULL);
		if(i >= g_recsec) {
			return false;
		}
	}

	return true;
}

static bool receive_from_tuner (void *p_recv_data, int length)
{
//	printf ("%p len:%d\n", p_recv_data, length);

	int r = OutputBuffer_put (g_pOutputBuffer, p_recv_data, length);
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
	g_recsec = args.recsec;
	g_destfile = args.destfile;

	ST_IT9175_SETUP_INFO setup_info = {
		pre_receive,
		post_receive,
		check_receive_loop,
		receive_from_tuner,
		NULL,
	};

	it9175_extern_setup (&setup_info);
	it9175_set_log_verbose (is_log_verbose);


	if (it9175_open ()) {
		it9175_tune (args.freq);
	}
	it9175_close ();


	exit (EXIT_SUCCESS);
}
