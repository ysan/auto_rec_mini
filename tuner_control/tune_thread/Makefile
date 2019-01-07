#
#   Defines
#
BASEDIR		:=	../../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR) \
	-I$(BASEDIR)/threadmgr \
	-I$(BASEDIR)/threadmgrpp \
	-I$(BASEDIR)/common \
	-I./ \
	-I../ \
	-I../it9175 \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L../it9175 -lit9175 \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \


#
#   Target object
#
TARGET_NAME	:=	tune_thread

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
