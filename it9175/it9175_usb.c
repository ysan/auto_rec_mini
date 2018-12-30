/* fsusb2i   (c) 2015-2016 trinity19683
  IT9175 USB commands (Linux OSes)
  it9175_usb.c
  2016-01-05
*/

#include <stddef.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

#include "osdepend.h"
#include "it9175_usb.h"
#include "it9175_priv.h"
#include "message.h"


static unsigned int  it9175_checksum(uint8_t* const buf, const int mode)
{
	int len, i;
	unsigned checksum = 0;
	len = buf[0] - 1;
	for(i = 1; i < len; i++) {
		checksum += (i & 1) ? buf[i] << 8 : buf[i];
	}
	checksum = ~checksum & 0xFFFF;
	if(0 != mode) {
		//# verify checksum
		return (buf[len] << 8 | buf[len +1]) ^ checksum;
	}
	//# add checksum
	buf[len] = checksum >> 8;
	buf[len +1] = checksum & 0xFF;
	return 0;
}

int it9175_ctrl_msg(void* const pst, const uint8_t cmd, const uint8_t mbox, const uint8_t wrlen, const uint8_t rdlen)
{
	struct state_st* const st = pst;
	int ret, wlen, rlen;
	struct usbdevfs_bulktransfer usbbulk;

	wlen = REQ_HDR_LEN + wrlen + CHECKSUM_LEN;
	if(CMD_FW_DL == cmd) {
		//# no ACK for these packets
		rlen = 0;
	}else{
		rlen = ACK_HDR_LEN + rdlen + CHECKSUM_LEN;
		if(MAX_XFER_SIZE < rlen) {
			warn_info(0,"buffer overflow");
			return -EINVAL;
		}
	}
	st->buf[0] = (wlen - 1) & 0xFF;
	st->buf[1] = mbox;
	st->buf[2] = cmd;

	//# mutex lock
	if((ret = uthread_mutex_lock(st->pmutex))) {
		warn_info(ret,"mutex_lock failed");
		return -EINVAL;
	}
	st->buf[3] = st->seq++;
	it9175_checksum(st->buf, 0);    //# calc and add checksum
	usbbulk.ep = EP_CTRLBULK;
	usbbulk.len = wlen;
	usbbulk.timeout = USB_TIMEOUT;
	usbbulk.data = st->buf;
	//# send cmd
	if(ioctl(st->fd, USBDEVFS_BULK, &usbbulk) < 0) {
		ret = -errno;
		warn_msg(errno,"CTRLcmd=%02X",cmd);
		goto exit_err0;
	}
	if(usbbulk.len != wlen) {
		ret = -EIO;
		warn_msg(usbbulk.len,"CTRLcmd=%02X wlen=%d",cmd,wlen);
		goto exit_err0;
	}

	if(0 < rlen) {
		usbbulk.ep = EP_CTRLBULKRES;
		usbbulk.len = rlen;
		usbbulk.timeout = USB_TIMEOUT;
		usbbulk.data = st->buf;
		//# read response
		if(ioctl(st->fd, USBDEVFS_BULK, &usbbulk) < 0) {
			ret = -errno;
			warn_msg(errno,"CTRLcmd=%02X_R",cmd);
			goto exit_err0;
		}
		if(usbbulk.len != rlen) {
			ret = -EIO;
			warn_msg(usbbulk.len,"CTRLcmd=%02X rlen=%d",cmd,rlen);
			goto exit_err0;
		}
	}

	//# mutex unlock
	if((ret = uthread_mutex_unlock(st->pmutex))) {
		warn_info(ret,"mutex_unlock failed");
		return -EINVAL;
	}

	if(it9175_checksum(st->buf, 1) != 0) {
		//# verify checksum, mismatch
		warn_msg(0,"CTRLcmd=%02X checksum mismatch",cmd);
		return -EIO;
	}

	if(st->buf[2]) {
		//# check status
		warn_msg(st->buf[2],"CTRLcmd=%02X",cmd);
		return -EIO;
	}

	return 0;
exit_err0:
	//# mutex unlock
	if((rlen = uthread_mutex_unlock(st->pmutex))) {
		warn_info(rlen,"mutex_unlock failed");
		return -EINVAL;
	}
	return ret;
}

int it9175_usbSetTimeout(void* const pst)
{ return 0; }

/*EOF*/