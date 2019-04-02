#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "usbdevfile.h"
#include "it9175.h"
#include "tsthread.h"
#include "utils.h"

#include "verstr.h"
#include "message.h"

#include "it9175_extern.h"


#define TS_FILE		"../in.m2ts"
//#define TS_FILE		"../test.m2ts"

int readFile (int fd, uint8_t *pBuff, size_t nSize)
{
	if ((!pBuff) || (nSize == 0)) {
		return -1;
	}

	int nReadSize = 0;
	int nDone = 0;

	while (1) {
		nReadSize = read (fd, pBuff, nSize);
		if (nReadSize < 0) {
			perror ("read()");
			return -1;

		} else if (nReadSize == 0) {
			// file end
			break;

		} else {
			// read done
			pBuff += nReadSize;
			nSize -= nReadSize;
			nDone += nReadSize;

			if (nSize == 0) {
				break;
			}
		}
	}

	return nDone;
}


//
// variables
//
static EN_IT9175_STATE g_en_state = EN_IT9175_STATE__CLOSED;

static struct usb_endpoint_st g_usbep;
static it9175_state g_devst = NULL;

static ST_IT9175_TS_RECEIVE_CALLBACKS g_ts_callbacks = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static bool g_is_log_verbose = false;
static bool g_is_force_tune_end = false;


//
// prototypes
//
void it9175_extern_setup_ts_callbacks (const ST_IT9175_TS_RECEIVE_CALLBACKS *p_callbacks); // extern
EN_IT9175_STATE it9175_get_state (void); // extern
void it9175_set_log_verbose (bool is_log_verbose); // extern
bool it9175_open (void ); // extern
void it9175_close (void); // extern
int it9175_tune (unsigned int freqKHz); // extern
void it9175_force_tune_end (void); // extern
static void printTMCC(const it9175_state state);
static int check_usbdevId(const unsigned int* desc);


void it9175_extern_setup_ts_callbacks (const ST_IT9175_TS_RECEIVE_CALLBACKS *p_callbacks)
{
	if (p_callbacks) {
		g_ts_callbacks = *p_callbacks;
	}
}

EN_IT9175_STATE it9175_get_state (void)
{
	return g_en_state;
}

void it9175_set_log_verbose (bool is_log_verbose)
{
	g_is_log_verbose = is_log_verbose;
}

bool it9175_open (void)
{
	int i = 0;
	struct timespec time_begin;
	struct timespec time_end;


	if (g_en_state == EN_IT9175_STATE__OPENED) {
		warn_info (0,"already opened.");
		return true;
	}

	if (g_en_state != EN_IT9175_STATE__CLOSED) {
		warn_msg (0,"invalid state.");
		return false;
	}

//	char *p_devfile = NULL;
//	memset (&g_usbep, 0x00, sizeof(g_usbep));
//	g_usbep.fd = usbdevfile_alloc(check_usbdevId, &p_devfile);
//	if(0 <= g_usbep.fd) {
//		msg("# ISDB-T TV Tuner FSUSB2i: '%s'\n", p_devfile);
//		free(p_devfile);
//	}else{
//		warn_msg(0,"No device can be used.");
//		return false;
//	}

	g_en_state = EN_IT9175_STATE__OPENED;

	u_gettime(&time_begin);

//	if(it9175_create(&g_devst, &g_usbep) != 0) {
//		return false;
//	}

	u_gettime(&time_end);
	u_difftime(&time_begin, &time_end, NULL, &i);
	if (g_is_log_verbose) {
		u_difftime(&time_begin, &time_end, NULL, &i);
		msg("init: %ums, ", i);
	}

	return true;
}

void it9175_close (void)
{
	if (g_en_state == EN_IT9175_STATE__CLOSED) {
		warn_info (0,"already close.");
		return ;
	}

	if ((g_en_state != EN_IT9175_STATE__TUNE_ENDED) && (g_en_state != EN_IT9175_STATE__OPENED)) {
		warn_msg (0,"invalid state.");
		return ;
	}

//	if (!g_devst) {
//		warn_msg (0,"invalid params.");
//		return ;
//	}

//	it9175_destroy (g_devst);

//	if(close(g_usbep.fd) == -1) {
//		warn_msg(errno,"failed to close usbdevfile");
//	}

//	memset (&g_usbep, 0x00, sizeof(g_usbep));
//	g_devst = NULL;

	g_en_state = EN_IT9175_STATE__CLOSED;
}

int it9175_tune (unsigned int freqKHz)
{
	int i = 0;
	int j = 0;
	int ret = 0;
//	tsthread_ptr tsthr;
	struct timespec time_begin;
	struct timespec time_end;

	if (
		(g_en_state == EN_IT9175_STATE__TUNE_BEGINED) ||
		(g_en_state == EN_IT9175_STATE__TUNED) || 
		(g_en_state == EN_IT9175_STATE__TUNE_ENDED) 
	) {
		warn_msg (0,"already tuning.");
		return 0;
	}

	if (g_en_state != EN_IT9175_STATE__OPENED) {
		warn_msg (0,"invalid state.");
		return 10;
	}

//	if (!g_devst) {
//		warn_msg (0,"invalid params.");
//		return 10;
//	}

	warn_info (0,"freq=%u kHz tuning...", freqKHz);

	g_en_state = EN_IT9175_STATE__TUNE_BEGINED;

//	for(i = 3; i > 0; i--) {  //# try 3 times
//		if(it9175_setFreq(g_devst, freqKHz) != 0) {
//			ret = 42;
//			goto _END;
//		}
//		u_gettime(&time_begin);
//		usleep(80000);
//		ret = it9175_waitTuning(g_devst, 1500);
//		u_gettime(&time_end);
//		if(ret < 0) {
//			warn_msg(ret,"failed to setFreq");
//			ret = 43;
//			goto _END;
//		}
//
//		if (g_is_log_verbose) {
//			u_difftime(&time_begin, &time_end, NULL, &j);
//			msg("waitTuning: %ums, ", j);
//		}
//		if((ret & 0x3) == 0x1) {
//			//# channel detected
//			break;
//		}else{
//			warn_msg(0,"freq=%u kHz is empty.", freqKHz);
//		}
//	}

	u_gettime(&time_begin);
//	ret = it9175_waitStream(g_devst, 1000);
//	u_gettime(&time_end);
//	if(0 > ret ||(0 == i && (ret & 0x1) == 0)) {
//		warn_msg( (0 > ret) ? ret : 0, "TS SYNC_LOCK failed");
//		ret = 44;
//		goto _END;
//	}

	if (g_is_log_verbose) {
		u_difftime(&time_begin, &time_end, NULL, &j);
		msg("waitStream: %ums, SYNC_LOCK=%c\n", j, (ret & 0x1)? 'Y':'N' );
	}
//	printTMCC(g_devst);

//	if(tsthread_create(&tsthr, &g_usbep) != 0) {
//		ret = 50;
//		goto _END_THREAD_DESTROY;
//	}

	//# purge TS buffer data
//	usleep(50000);
//	for(i = 0; i < 5; i++) {
//		for(j = 0; j < 500; j++) {
//			if(tsthread_read(tsthr, NULL) <= 0)  break;
//		}
//		ret = it9175_waitStream(g_devst, 30);
//		if(ret < 0 || (ret & 0x2) == 0)  break;
//	}

	g_en_state = EN_IT9175_STATE__TUNED;

	if (g_ts_callbacks.pcb_pre_ts_receive) {
		if (!g_ts_callbacks.pcb_pre_ts_receive (g_ts_callbacks.p_shared_data)) {
			goto _END_THREAD_STOP;
		}
	}

	//# main loop
	msg("# Start!\n");
//	while (1) {
//		if (g_is_force_tune_end) {
//			g_is_force_tune_end = false;
//			break;
//		}
//		if (g_ts_callbacks.pcb_check_ts_receive_loop) {
//			if (!g_ts_callbacks.pcb_check_ts_receive_loop (g_ts_callbacks.p_shared_data)) {
//				break;
//			}
//		}
//
//		void* pBuffer;
//		int rlen;
//		if((rlen = tsthread_read(tsthr, &pBuffer)) > 0) {
//			if (g_ts_callbacks.pcb_ts_received) {
//				if (!g_ts_callbacks.pcb_ts_received (g_ts_callbacks.p_shared_data, pBuffer, rlen)) {
//					break;
//				}
//			}
//
//		}else{
//			static unsigned  countVerboseMsg = 0;
//			countVerboseMsg ++;
//			usleep(250000);
//			if(g_is_log_verbose && 3 <= countVerboseMsg ) {  //# verbose: status message
//				uint8_t statData[44];
//				countVerboseMsg = 0;
//				if(it9175_readStatistic(g_devst, statData) == 0) {
//					if(0x3 != statData[0])  msg("SignalLock=%X, ", statData[0]);
//					msg("Stre= %3ddBm, SNR= %2udB, Qual=%3u%%, ", (int8_t)statData[4], statData[3], statData[1]);
//#ifdef DEBUG
//					uint32_t* pval = (uint32_t*)(statData + 8);
//					for(i = 0; i < 3; i++) {
//						dmsgn("%c: %.2e %u, ", i + 'A', pval[0] / (double)pval[1], pval[2]);
//						pval += 3;
//					}
//#endif
//				}
//				msg("\n");
//			}
//		}
//	}

	int fd = 0;
	int readSize = 0;
	int readTotal = 0;
	uint8_t buff [65536] = {0};

	fd = open (TS_FILE, O_RDONLY);
	if (fd < 0) {
		perror ("open");
		exit (EXIT_FAILURE);
	}
//fd = STDIN_FILENO;

	while (1) {
		if (g_is_force_tune_end) {
			g_is_force_tune_end = false;
			break;
		}
		if (g_ts_callbacks.pcb_check_ts_receive_loop) {
			if (!g_ts_callbacks.pcb_check_ts_receive_loop (g_ts_callbacks.p_shared_data)) {
				break;
			}
		}

		memset (buff, 0x00, sizeof(buff));
		readSize = readFile (fd, buff, sizeof(buff));
		if (readSize < 0) {
			fprintf (stdout, "CUtils::readFile() is failure.\n");
		} else if (readSize == 0) {
			break;
		}

		if (g_ts_callbacks.pcb_ts_received) {
			if (!g_ts_callbacks.pcb_ts_received (g_ts_callbacks.p_shared_data, buff, readSize)) {
				break;
			}
		}

		readTotal += readSize;

printf ("%d\n", readSize);
	}

	ret = 0;

	if (g_ts_callbacks.pcb_post_ts_receive) {
		g_ts_callbacks.pcb_post_ts_receive (g_ts_callbacks.p_shared_data);
	}
sleep (1);

_END_THREAD_STOP:
//	tsthread_stop(tsthr);

_END_THREAD_DESTROY:
//	tsthread_destroy(tsthr);

_END:
	g_en_state = EN_IT9175_STATE__TUNE_ENDED;

	return ret;
}

void it9175_force_tune_end (void)
{
	if (g_en_state == EN_IT9175_STATE__TUNED) {
		g_is_force_tune_end = true;
	}
}

static void printTMCC(const it9175_state state)
{
	const char * const s_carmode[5] = {"DQPSK","QPSK","16QAM","64QAM","??"};
	const char * const s_crmode[6] = {"1/2","2/3","3/4","5/6","7/8","??"};
	uint8_t bf[16], *ptr = bf, j;

	if( it9175_readTMCC(state, bf) )  return;
	msg("# TMCC: TXmode=%u GuardInterval=1/%u OneSeg=%c\n", bf[0], bf[1], (bf[2])? 'Y':'N' );
	for( j = 0; j < 3; j++ ) {
		ptr += 4;
		if(! ptr[0])  continue;
msg("%c: %-6s %2useg FEC=%-3s IL=%-2u\n", j + 'A', s_carmode[ ptr[1] ], ptr[0], s_crmode[ ptr[2] ], ptr[3]);
	}
}

static int check_usbdevId(const unsigned int* desc)
{
	//if(0x048d == desc[0] && 0x9175 == desc[1])  return 0;
	if(0x0511 == desc[0] && 0x0046 == desc[1])  return 0;
	return 1;   //# not match
}
