#
#   Defines
#
BASEDIR		:=	../../
ROOTDIR		:=	$(BASEDIR)/src

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR)/third_party/coseq/coseq \
	-I$(ROOTDIR)/third_party/coseq/coseqpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/event_schedule_manager \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/parser/third_party/aribstr \
	-I$(ROOTDIR)/parser/psisi \
	-I$(ROOTDIR)/parser/psisi/descriptor \
	-I$(ROOTDIR)/parser/dsmcc \
	-I$(ROOTDIR)/parser/dsmcc/dsmcc_descriptor \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(ROOTDIR)/third_party/coseq/coseq -lcoseq \
	-L$(ROOTDIR)/third_party/coseq/coseqpp -lcoseqpp \
	-L$(ROOTDIR)/common -lcommon \
	-L$(ROOTDIR)/tuner_control -ltuner_control \
	-L$(ROOTDIR)/parser -lparser \
	-L$(ROOTDIR)/parser/third_party/aribstr -laribstr \
	-L$(ROOTDIR)/parser/psisi -lpsisiparser \
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
	psisi_manager.cpp \
	psisi_manager_commands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
