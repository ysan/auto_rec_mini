#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11
ifeq ($(shell getconf LONG_BIT),32)
CFLAGS		+=	-D_FILE_OFFSET_BITS=64
endif

INCLUDES	:= \
	-I$(BASEDIR)/ \
	-I./ \

LIBS		:= \
	-lpthread \

SUBDIRS		:= \

#
#   Target object
#
TARGET_NAME	:=	common

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS		:=

SRCS_CPP	:= \
	Utils.cpp \
	Etime.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

