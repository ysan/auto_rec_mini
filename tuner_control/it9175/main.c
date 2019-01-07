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

#include "usbdevfile.h"
#include "it9175.h"
#include "tsthread.h"
#include "utils.h"

#include "verstr.h"
#include "message.h"


static void printTMCC(const it9175_state);
static unsigned char volatile  caughtSignal = 0;
static void sighandler(int arg)    { caughtSignal = 1; }

static int check_usbdevId(const unsigned int* desc)
{
	//if(0x048d == desc[0] && 0x9175 == desc[1])  return 0;
	if(0x0511 == desc[0] && 0x0046 == desc[1])  return 0;
	return 1;   //# not match
}


int main(int argc, char **argv)
{
	int ret, i, j;
	struct Args args;
	struct timespec  time_begin, time_end;
	struct usb_endpoint_st  usbep;
	it9175_state devst = NULL;
	tsthread_ptr tsthr;

	msg("recfsusb2i " PRG_VER " " PRG_BUILT " " PRG_TIMESTAMP ", (GPL) 2015-2016 trinity19683\n");
	parseOption(argc, argv, &args);

	usbep.fd = usbdevfile_alloc(check_usbdevId, &args.devfile);
	if(0 <= usbep.fd) {
		msg("# ISDB-T TV Tuner FSUSB2i: '%s', freq= %u kHz, duration= %u sec.\n", args.devfile, args.freq, args.recsec);
		free(args.devfile);
	}else{
		warn_msg(0,"No device can be used.");
		ret = 40;	goto end1;
	}
	u_gettime(&time_begin);
	if(it9175_create(&devst, &usbep) != 0) {
		ret = 41;	goto end3;
	}
	u_gettime(&time_end);
	u_difftime(&time_begin, &time_end, NULL, &j);
	if(args.flags & 0x1) {  //# verbose
		u_difftime(&time_begin, &time_end, NULL, &j);
		msg("init: %ums, ", j);
	}

	for(i = 3; i > 0; i--) {  //# try 3 times
		if(it9175_setFreq(devst, args.freq) != 0) {
			ret = 42;	goto end3;
		}
		u_gettime(&time_begin);
		usleep(80000);
		ret = it9175_waitTuning(devst, 1500);
		u_gettime(&time_end);
		if(ret < 0) {
			warn_msg(ret,"failed to setFreq");
			ret = 43;	goto end3;
		}
		if(args.flags & 0x1) {  //# verbose
			u_difftime(&time_begin, &time_end, NULL, &j);
			msg("waitTuning: %ums, ", j);
		}
		if((ret & 0x3) == 0x1) {
			//# channel detected
			break;
		}else{
			warn_msg(0,"freq=%u kHz is empty.", args.freq);
		}
	}

	u_gettime(&time_begin);
	ret = it9175_waitStream(devst, 1000);
	u_gettime(&time_end);
	if(0 > ret ||(0 == i && (ret & 0x1) == 0)) {
		warn_msg( (0 > ret) ? ret : 0, "TS SYNC_LOCK failed");
		ret = 44;	goto end3;
	}
	if(args.flags & 0x1) {  //# verbose
		u_difftime(&time_begin, &time_end, NULL, &j);
		msg("waitStream: %ums, SYNC_LOCK=%c\n", j, (ret & 0x1)? 'Y':'N' );
	}
	printTMCC(devst);

	if(tsthread_create(&tsthr, &usbep) != 0) {
		ret = 50;	goto end4;
	}
	//# purge TS buffer data
	usleep(50000);
	for(i = 0; i < 5; i++) {
		for(j = 0; j < 500; j++) {
			if(tsthread_read(tsthr, NULL) <= 0)  break;
		}
		ret = it9175_waitStream(devst, 30);
		if(ret < 0 || (ret & 0x2) == 0)  break;
	}
	//# OutputBuffer
	struct OutputBuffer *pOutputBuffer;
	pOutputBuffer = create_FileBufferedWriter( 768 * 1024, args.destfile );
	if(! pOutputBuffer ) {
		warn_msg(0,"failed to init FileBufferedWriter.");
		ret = 70;	goto end4;
	}else{
		struct OutputBuffer * const  pFileBufferedWriter = pOutputBuffer;
		j = (args.flags & 0x1000)? 0 : 1 ;
		pOutputBuffer = create_TSParser( 8192, pFileBufferedWriter, j );
		if(! pOutputBuffer ) {
			warn_msg(0,"failed to init TS Parser.");
			OutputBuffer_release(pFileBufferedWriter);
			ret = 71;	goto end4;
		}
	}
	//# change signal handler
	setSignalHandler(1, sighandler);
	//# time of the begin of rec.
	u_gettime(&time_begin);

	//# main loop
	msg("# Start!\n");
	while(! caughtSignal) {
		if(0 < args.recsec) {
			u_gettime(&time_end);
			u_difftime(&time_begin, &time_end, &j, NULL);
			if(j >= args.recsec)  break;
		}

		void* pBuffer;
		int rlen;
		if((rlen = tsthread_read(tsthr, &pBuffer)) > 0) {
			if( 0 < rlen && (ret = OutputBuffer_put(pOutputBuffer, pBuffer, rlen) ) < 0){
				warn_msg(ret,"TS write failed");
			}
		}else{
			static unsigned  countVerboseMsg = 0;
			countVerboseMsg ++;
			usleep(250000);
			if((args.flags & 0x1) && 3 <= countVerboseMsg ) {  //# verbose: status message
				uint8_t statData[44];
				countVerboseMsg = 0;
				if(it9175_readStatistic(devst, statData) == 0) {
					if(0x3 != statData[0])  msg("SignalLock=%X, ", statData[0]);
					msg("Stre= %3ddBm, SNR= %2udB, Qual=%3u%%, ", (int8_t)statData[4], statData[3], statData[1]);
#ifdef DEBUG
					uint32_t* pval = (uint32_t*)(statData + 8);
					for(i = 0; i < 3; i++) {
						dmsgn("%c: %.2e %u, ", i + 'A', pval[0] / (double)pval[1], pval[2]);
						pval += 3;
					}
#endif
				}
				msg("\n");
			}
		}
	}
	ret = 0;

	//# restore signal handler
	setSignalHandler(0, sighandler);
	//# flush data stream
	tsthread_stop(tsthr);
	if( pOutputBuffer ) {
		OutputBuffer_flush(pOutputBuffer);
		OutputBuffer_release(pOutputBuffer);
	}
	//# time of the end of rec.
	u_gettime(&time_end);
end4:
	tsthread_destroy(tsthr);
end3:
	it9175_destroy(devst);
//end2:
	if(close(usbep.fd) == -1) {
		warn_msg(errno,"failed to close usbdevfile");
	}
end1:
	if(0 == ret) {
		u_difftime(&time_begin, &time_end, &i, &j);
		msg("\n# Done! %u.%03u sec.\n", i, j);
	}else{
		warn_msg(ret,"\n# Abort!");
	}

	return ret;
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


/*EOF*/
