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
	-I$(BASEDIR)/ts_parser \
	-I$(BASEDIR)/ts_parser/aribstr \
	-I$(BASEDIR)/ts_parser/psisi \
	-I$(BASEDIR)/ts_parser/psisi/descriptor \
	-I$(BASEDIR)/ts_parser/dsmcc \
	-I$(BASEDIR)/ts_parser/dsmcc/dsmcc_descriptor \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L$(BASEDIR)/tuner_control -ltuner_control \
	-L$(BASEDIR)/ts_parser -lts_parser \
	-L$(BASEDIR)/ts_parser/aribstr -laribstr \
	-L$(BASEDIR)/ts_parser/psisi -lpsisi \
	-lpthread \

USERDEFS	:= \


#
#   Target object
#
TARGET_NAME	:=	psisi_manager

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
	TsParserListener.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
