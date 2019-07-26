#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR) \
	-I$(BASEDIR)/threadmgr \
	-I$(BASEDIR)/threadmgrpp \
	-I$(BASEDIR)/common \
	-I$(BASEDIR)/command_server \
	-I$(BASEDIR)/tuner_control \
	-I$(BASEDIR)/parser \
	-I$(BASEDIR)/rec_manager \
	-I$(BASEDIR)/event_schedule_manager \
	-I./ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \

#
#   Target object
#
TARGET_NAME	:=	eventsearch

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	EventSearch.cpp \
	EventSearchCommands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
