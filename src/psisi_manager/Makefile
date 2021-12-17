#
#   Defines
#
BASEDIR		:=	../../
ROOTDIR		:=	$(BASEDIR)/src

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR)/threadmgr \
	-I$(ROOTDIR)/threadmgrpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/event_schedule_manager \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/parser/aribstr \
	-I$(ROOTDIR)/parser/psisi \
	-I$(ROOTDIR)/parser/psisi/descriptor \
	-I$(ROOTDIR)/parser/dsmcc \
	-I$(ROOTDIR)/parser/dsmcc/dsmcc_descriptor \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(ROOTDIR)/threadmgr -lthreadmgr \
	-L$(ROOTDIR)/threadmgrpp -lthreadmgrpp \
	-L$(ROOTDIR)/common -lcommon \
	-L$(ROOTDIR)/tuner_control -ltunercontrol \
	-L$(ROOTDIR)/parser -lparser \
	-L$(ROOTDIR)/parser/aribstr -laribstr \
	-L$(ROOTDIR)/parser/psisi -lpsisiparser \
	-lpthread \

USERDEFS	:= \


#
#   Target object
#
TARGET_NAME	:=	psisimanager

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	PsisiManager.cpp \
	PsisiManagerCommands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
