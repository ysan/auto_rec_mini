#
#   Defines
#
BASEDIR		:=	../../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR)/third_party/coseq/coseq \
	-I$(ROOTDIR)/third_party/coseq/coseqpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/parser/third_party/aribstr \
	-I$(ROOTDIR)/parser/psisi \
	-I$(ROOTDIR)/parser/psisi/descriptor \
	-I$(ROOTDIR)/psisi_manager \
	-I$(ROOTDIR)/tuner_service \
	-I$(ROOTDIR)/channel_manager \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(ROOTDIR)/third_party/coseq/coseq -lcoseq \
	-L$(ROOTDIR)/third_party/coseq/coseqpp -lcoseqpp \
	-L$(ROOTDIR)/common -lcommon \
	-L$(ROOTDIR)/parser -lparser \
	-L$(ROOTDIR)/parser/third_party/aribstr -laribstr \
	-L$(ROOTDIR)/psisi_manager -lpsisi_manager \
	-L$(ROOTDIR)/tuner_service -ltuner_service \
	-lpthread \

USERDEFS	:= \


#
#   Target object
#
TARGET_NAME	:=	event_schedule_manager

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	event_schedule_manager.cpp \
	event_schedule_manager_commands.cpp \
	event_schedule_container.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
