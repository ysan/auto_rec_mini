#
#   Defines
#
BASEDIR		:=	../../../
ROOTDIR		:=	$(BASEDIR)/src

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR) \
	-I$(ROOTDIR)/threadmgr \
	-I$(ROOTDIR)/threadmgrpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/parser \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(ROOTDIR)/threadmgr -lthreadmgr \
	-L$(ROOTDIR)/threadmgrpp -lthreadmgrpp \
	-L$(ROOTDIR)/common -lcommon \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \


#
#   Target object
#
TARGET_NAME	:=	tunethread

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	TuneThread.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
