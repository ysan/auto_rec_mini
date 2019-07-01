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
	-I$(BASEDIR)/parser \
	-I$(BASEDIR)/parser/aribstr \
	-I$(BASEDIR)/parser/psisi \
	-I$(BASEDIR)/parser/psisi/descriptor \
	-I$(BASEDIR)/parser/dsmcc \
	-I$(BASEDIR)/parser/dsmcc/dsmcc_descriptor \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L$(BASEDIR)/tuner_control -ltunercontrol \
	-L$(BASEDIR)/parser -lparser \
	-L$(BASEDIR)/parser/aribstr -laribstr \
	-L$(BASEDIR)/parser/psisi -lpsisiparser \
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
