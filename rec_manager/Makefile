#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11
ifeq ($(shell getconf LONG_BIT),32)
CFLAGS		+=	-D_FILE_OFFSET_BITS=64
endif

INCLUDES	:= \
	-I$(BASEDIR)/threadmgr \
	-I$(BASEDIR)/threadmgrpp \
	-I$(BASEDIR)/common \
	-I$(BASEDIR)/command_server \
	-I$(BASEDIR)/tuner_control \
	-I$(BASEDIR)/tuner_service \
	-I$(BASEDIR)/parser \
	-I$(BASEDIR)/parser/aribstr \
	-I$(BASEDIR)/psisi_manager \
	-I$(BASEDIR)/channel_manager \
	-I$(BASEDIR)/event_schedule_manager \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L$(BASEDIR)/tuner_control -ltunercontrol \
	-L$(BASEDIR)/tuner_service -ltunerservice \
	-L$(BASEDIR)/parser -lparser \
	-L$(BASEDIR)/parser/aribstr -laribstr \
	-L$(BASEDIR)/psisi_manager -lpsisimanager \
	-lpthread \

USERDEFS	:= \


#
#   Target object
#
TARGET_NAME	:=	recmanager

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	RecManager.cpp \
	RecManagerCommands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
