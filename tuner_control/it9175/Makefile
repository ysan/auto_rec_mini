#
#   Defines
#
BASEDIR		:=	../../

CFLAGS		:=	-DDEBUG -O2 -Wall -DSTD_B25
ifeq ($(shell getconf LONG_BIT),32)
CFLAGS		+=	-D_FILE_OFFSET_BITS=64
endif

INCLUDES	:= \
	-I./ \
	-I$(BASEDIR) \

LIBS		:=	-lpthread -larib25

APPEND_OBJS :=

SUBDIRS		:=

#
#   Target object
#
TARGET_NAME	:=	it9175

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
ifeq ($(NO_TUNER), 1)
SRCS		:= \
	it9175_extern_debug.c \
	it9175.c \
	it9175_usb.c \
	message.c \
	osdepend.c \
	tsthread.c \
	usbdevfile.c \
	usbops.c \
	utils.c
else
SRCS		:= \
	it9175_extern.c \
	it9175.c \
	it9175_usb.c \
	message.c \
	osdepend.c \
	tsthread.c \
	usbdevfile.c \
	usbops.c \
	utils.c
endif


#
#   Configurations
#
include $(BASEDIR)/Config.mak

