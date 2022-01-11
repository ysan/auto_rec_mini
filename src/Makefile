#
#   Defines
#
BASEDIR		:=	../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR)/ \
	-I$(ROOTDIR)/threadmgr \
	-I$(ROOTDIR)/threadmgrpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/tuner_control/tune_thread \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/parser/aribstr \
	-I$(ROOTDIR)/parser/psisi \
	-I$(ROOTDIR)/parser/psisi/descriptor \
	-I$(ROOTDIR)/parser/dsmcc \
	-I$(ROOTDIR)/parser/dsmcc/dsmcc_descriptor \
	-I$(ROOTDIR)/psisi_manager \
	-I$(ROOTDIR)/tuner_service \
	-I$(ROOTDIR)/rec_manager \
	-I$(ROOTDIR)/channel_manager \
	-I$(ROOTDIR)/event_schedule_manager \
	-I$(ROOTDIR)/event_search \
	-I$(ROOTDIR)/command_server \
	-I./ \

LIBS		:= \
	-L$(ROOTDIR)/threadmgr -lthreadmgr \
	-L$(ROOTDIR)/threadmgrpp -lthreadmgrpp \
	-L$(ROOTDIR)/common -lcommon \
	-L$(ROOTDIR)/tuner_control/tune_thread -ltunethread \
	-L$(ROOTDIR)/tuner_control -ltunercontrol \
	-L$(ROOTDIR)/parser -lparser \
	-L$(ROOTDIR)/parser/psisi -lpsisiparser \
	-L$(ROOTDIR)/parser/dsmcc -ldsmccparser \
	-L$(ROOTDIR)/psisi_manager -lpsisimanager \
	-L$(ROOTDIR)/tuner_service -ltunerservice \
	-L$(ROOTDIR)/rec_manager -lrecmanager \
	-L$(ROOTDIR)/channel_manager -lchannelmanager \
	-L$(ROOTDIR)/event_schedule_manager -leventschedulemanager \
	-L$(ROOTDIR)/event_search -leventsearch \
	-L$(ROOTDIR)/command_server -lcommandserver \
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
	tuner_service \
	rec_manager \
	channel_manager \
	event_schedule_manager \
	event_search \
	command_server \


#
#   Target object
#
TARGET_NAME	:=	auto_rec_mini

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
