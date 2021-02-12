#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR)/threadmgr \
	-I$(BASEDIR)/threadmgrpp \
	-I$(BASEDIR)/common \
	-I$(BASEDIR)/command_server \
	-I$(BASEDIR)/parser \
	-I$(BASEDIR)/parser/aribstr \
	-I$(BASEDIR)/parser/psisi \
	-I$(BASEDIR)/parser/psisi/descriptor \
	-I$(BASEDIR)/psisi_manager \
	-I$(BASEDIR)/tuner_service \
	-I$(BASEDIR)/channel_manager \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L$(BASEDIR)/parser -lparser \
	-L$(BASEDIR)/parser/aribstr -laribstr \
	-L$(BASEDIR)/psisi_manager -lpsisimanager \
	-L$(BASEDIR)/tuner_service -ltunerservice \
	-lpthread \

USERDEFS	:= \


#
#   Target object
#
TARGET_NAME	:=	eventschedulemanager

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	EventScheduleManager.cpp \
	EventScheduleManagerCommands.cpp \
	EventScheduleContainer.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
