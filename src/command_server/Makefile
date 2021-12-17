#
#   Defines
#
BASEDIR		:=	../../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR)/threadmgr \
	-I$(ROOTDIR)/threadmgrpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/psisi_manager \
	-I$(ROOTDIR)/tuner_service \
	-I$(ROOTDIR)/rec_manager \
	-I$(ROOTDIR)/channel_manager \
	-I$(ROOTDIR)/event_schedule_manager \
	-I$(ROOTDIR)/event_search \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(ROOTDIR)/threadmgr -lthreadmgr \
	-L$(ROOTDIR)/threadmgrpp -lthreadmgrpp \
	-L$(ROOTDIR)/common -lcommon \
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
	CommandServerLog.cpp \
	CommandTables.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
