#
#   Defines
#
BASEDIR		:=	../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11 -Wno-psabi

INCLUDES	:= \
	-I$(ROOTDIR)/ \
	-I$(ROOTDIR)/third_party/coseq/coseq \
	-I$(ROOTDIR)/third_party/coseq/coseqpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/tuner_control/tune_thread \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/parser/third_party/aribstr \
	-I$(ROOTDIR)/parser/third_party/tssplitter_lite \
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
	-I$(ROOTDIR)/viewing_manager \
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/stream_handler \
	-I./ \

LIBS		:= \
	-L$(ROOTDIR)/third_party/coseq/coseq -lcoseq \
	-L$(ROOTDIR)/third_party/coseq/coseqpp -lcoseqpp \
	-L$(ROOTDIR)/common -lcommon \
	-L$(ROOTDIR)/tuner_control/tune_thread -ltune_thread \
	-L$(ROOTDIR)/tuner_control -ltuner_control \
	-L$(ROOTDIR)/parser -lparser \
	-L$(ROOTDIR)/parser/psisi -lpsisiparser \
	-L$(ROOTDIR)/parser/dsmcc -ldsmccparser \
	-L$(ROOTDIR)/psisi_manager -lpsisi_manager \
	-L$(ROOTDIR)/tuner_service -ltuner_service \
	-L$(ROOTDIR)/rec_manager -lrec_manager \
	-L$(ROOTDIR)/channel_manager -lchannel_manager \
	-L$(ROOTDIR)/event_schedule_manager -levent_schedule_manager \
	-L$(ROOTDIR)/event_search -levent_search \
	-L$(ROOTDIR)/viewing_manager -lviewing_manager \
	-L$(ROOTDIR)/command_server -lcommand_server \
	-L$(ROOTDIR)/stream_handler -lstream_handler \
	-larib25 \
	-lpthread \
	-lpcsclite \

SUBDIRS		:= \
	third_party/coseq/coseq \
	third_party/coseq/coseqpp \
	common \
	parser \
	parser_test \
	stream_handler \
	tuner_control \
	psisi_manager \
	tuner_service \
	rec_manager \
	channel_manager \
	event_schedule_manager \
	event_search \
	viewing_manager \
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

