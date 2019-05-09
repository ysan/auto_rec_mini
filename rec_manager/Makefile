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
	-I$(BASEDIR)/tuner_control \
	-I$(BASEDIR)/tuner_control/it9175 \
	-I$(BASEDIR)/ts_parser \
	-I$(BASEDIR)/ts_parser/aribstr \
	-I$(BASEDIR)/psisi_manager \
	-I$(BASEDIR)/channel_manager \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L$(BASEDIR)/tuner_control -ltunercontrol \
	-L$(BASEDIR)/ts_parser -ltsparser \
	-L$(BASEDIR)/ts_parser/aribstr -laribstr \
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
