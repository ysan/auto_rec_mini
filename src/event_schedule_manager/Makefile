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
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/parser/aribstr \
	-I$(ROOTDIR)/parser/psisi \
	-I$(ROOTDIR)/parser/psisi/descriptor \
	-I$(ROOTDIR)/psisi_manager \
	-I$(ROOTDIR)/tuner_service \
	-I$(ROOTDIR)/channel_manager \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(ROOTDIR)/threadmgr -lthreadmgr \
	-L$(ROOTDIR)/threadmgrpp -lthreadmgrpp \
	-L$(ROOTDIR)/common -lcommon \
	-L$(ROOTDIR)/parser -lparser \
	-L$(ROOTDIR)/parser/aribstr -laribstr \
	-L$(ROOTDIR)/psisi_manager -lpsisimanager \
	-L$(ROOTDIR)/tuner_service -ltunerservice \
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
