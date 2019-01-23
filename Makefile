#
#   Defines
#
BASEDIR		:=	./

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR)/ \
	-I$(BASEDIR)/threadmgr \
	-I$(BASEDIR)/threadmgrpp \
	-I$(BASEDIR)/common \
	-I$(BASEDIR)/tuner_control/tune_thread \
	-I$(BASEDIR)/tuner_control \
	-I$(BASEDIR)/tuner_control/it9175 \
	-I$(BASEDIR)/ts_parser \
	-I$(BASEDIR)/ts_parser/psisi \
	-I$(BASEDIR)/ts_parser/psisi/descriptor \
	-I$(BASEDIR)/ts_parser/dsmcc \
	-I$(BASEDIR)/ts_parser/dsmcc/dsmcc_descriptor \
	-I$(BASEDIR)/psisi_manager \
	-I$(BASEDIR)/command_server \
	-I./ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L$(BASEDIR)/tuner_control/tune_thread -ltune_thread \
	-L$(BASEDIR)/tuner_control -ltuner_control \
	-L$(BASEDIR)/tuner_control/it9175 -lit9175 \
	-L$(BASEDIR)/ts_parser -lts_parser \
	-L$(BASEDIR)/psisi_manager -lpsisi_manager \
	-L$(BASEDIR)/command_server -lcommand_server \
	-larib25 \
	-lpthread \
	-lpcsclite \

SUBDIRS		:= \
	threadmgr \
	threadmgrpp \
	common \
	ts_parser \
	ts_parser_test \
	tuner_control \
	psisi_manager \
	command_server \


#
#   Target object
#
TARGET_NAME	:=	atpp

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	EXEC

#
#   Compile sources
#
SRCS_CPP	:= \
	modules.cpp \
	main.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

