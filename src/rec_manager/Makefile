#
#   Defines
#
BASEDIR		:=	../../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11
ifeq ($(shell getconf LONG_BIT),32)
CFLAGS		+=	-D_FILE_OFFSET_BITS=64
endif

INCLUDES	:= \
	-I$(ROOTDIR)/third_party/coseq/coseq \
	-I$(ROOTDIR)/third_party/coseq/coseqpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/tuner_service \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/parser/third_party/aribstr \
	-I$(ROOTDIR)/parser/third_party/tssplitter_lite \
	-I$(ROOTDIR)/psisi_manager \
	-I$(ROOTDIR)/channel_manager \
	-I$(ROOTDIR)/event_schedule_manager \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(ROOTDIR)/third_party/coseq/coseq -lcoseq \
	-L$(ROOTDIR)/third_party/coseq/coseqpp -lcoseqpp \
	-L$(ROOTDIR)/common -lcommon \
	-L$(ROOTDIR)/tuner_control -ltuner_control \
	-L$(ROOTDIR)/tuner_service -ltuner_service \
	-L$(ROOTDIR)/parser -lparser \
	-L$(ROOTDIR)/parser/third_party/aribstr -laribstr \
	-L$(ROOTDIR)/parser/third_party/tssplitter_lite -ltssplitter_lite \
	-L$(ROOTDIR)/psisi_manager -lpsisi_manager \
	-lpthread \

USERDEFS	:= \


#
#   Target object
#
TARGET_NAME	:=	rec_manager

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	rec_manager.cpp \
	rec_manager_commands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
