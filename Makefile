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
	-I$(BASEDIR)/parser \
	-I$(BASEDIR)/parser/aribstr \
	-I$(BASEDIR)/parser/psisi \
	-I$(BASEDIR)/parser/psisi/descriptor \
	-I$(BASEDIR)/parser/dsmcc \
	-I$(BASEDIR)/parser/dsmcc/dsmcc_descriptor \
	-I$(BASEDIR)/psisi_manager \
	-I$(BASEDIR)/rec_manager \
	-I$(BASEDIR)/channel_manager \
	-I$(BASEDIR)/event_schedule_manager \
	-I$(BASEDIR)/event_search \
	-I$(BASEDIR)/command_server \
	-I./ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L$(BASEDIR)/tuner_control/tune_thread -ltunethread \
	-L$(BASEDIR)/tuner_control -ltunercontrol \
	-L$(BASEDIR)/tuner_control/it9175 -lit9175 \
	-L$(BASEDIR)/parser -lparser \
	-L$(BASEDIR)/parser/psisi -lpsisiparser \
	-L$(BASEDIR)/parser/dsmcc -ldsmccparser \
	-L$(BASEDIR)/psisi_manager -lpsisimanager \
	-L$(BASEDIR)/rec_manager -lrecmanager \
	-L$(BASEDIR)/channel_manager -lchannelmanager \
	-L$(BASEDIR)/event_schedule_manager -leventschedulemanager \
	-L$(BASEDIR)/event_search -leventsearch \
	-L$(BASEDIR)/command_server -lcommandserver \
	-larib25 \
	-lpthread \
	-lpcsclite \

SUBDIRS		:= \
	threadmgr \
	threadmgrpp \
	common \
	parser \
	parser_test \
	tuner_control \
	psisi_manager \
	rec_manager \
	channel_manager \
	event_schedule_manager \
	event_search \
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

