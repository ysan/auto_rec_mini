/* fsusb2i   (c) 2015-2016 trinity19683
  type definition (Linux OSes)
  types_u.h
  2016-02-12
*/
#pragma once

typedef int HANDLE;
typedef void* PMUTEX;

struct usb_endpoint_st {
	HANDLE fd;    //# devfile descriptor
	unsigned  endpoint;    //# USB endpoint
	void* dev;
	int (* startstopFunc)(void * const  dev, const int start);
	unsigned  xfer_size;    //# transfer unit size
};

/*EOF*/