#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR)/threadmgr \
	-I$(BASEDIR)/threadmgrpp \
	-I$(BASEDIR)/common \
	-I$(BASEDIR)/tuner_control \
	-I$(BASEDIR)/tuner_control/it9175 \
	-I$(BASEDIR)/psisi_manager \
	-I$(BASEDIR)/rec_manager \
	-I$(BASEDIR)/channel_manager \
	-I$(BASEDIR)/event_schedule_manager \
	-I$(BASEDIR)/event_search \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-lpthread \

USERDEFS	:= \


#
#   Target object
#
TARGET_NAME	:=	commandserver

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	CommandServer.cpp \
	CommandTables.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
