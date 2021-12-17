#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


//TODO
#define _NO_TYPEDEF_uint64_t


#if !defined (_NO_TYPEDEF_uint8_t)
typedef unsigned char uint8_t;
#endif

#if !defined (_NO_TYPEDEF_uint16_t)
typedef unsigned short uint16_t;
#endif

#if !defined (_NO_TYPEDEF_uint32_t)
typedef unsigned int uint32_t;
#endif

//TODO
#ifndef _ANDROID_BUILD
#if !defined (_NO_TYPEDEF_uint64_t)
typedef unsigned long int uint64_t;
#endif
#endif

//#define __PRETTY_FUNCTION__     __func__


#endif
