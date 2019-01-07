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
	-I$(BASEDIR)/command_server \
	-I./ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L$(BASEDIR)/tuner_control/tune_thread -ltune_thread \
	-L$(BASEDIR)/tuner_control -ltuner_control \
	-L$(BASEDIR)/tuner_control/it9175 -lit9175 \
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

