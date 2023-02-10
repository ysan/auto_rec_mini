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
	-I$(ROOTDIR)/threadmgr \
	-I$(ROOTDIR)/threadmgrpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/tuner_service \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/parser/aribstr \
	-I$(ROOTDIR)/parser/tssplitter_lite \
	-I$(ROOTDIR)/psisi_manager \
	-I$(ROOTDIR)/channel_manager \
	-I$(ROOTDIR)/event_schedule_manager \
	-I$(ROOTDIR)/stream_handler \
	-I./ \
	-I../ \

LIBS		:= \
	-L$(ROOTDIR)/threadmgr -lthreadmgr \
	-L$(ROOTDIR)/threadmgrpp -lthreadmgrpp \
	-L$(ROOTDIR)/common -lcommon \
	-L$(ROOTDIR)/tuner_control -ltunercontrol \
	-L$(ROOTDIR)/tuner_service -ltunerservice \
	-L$(ROOTDIR)/parser -lparser \
	-L$(ROOTDIR)/parser/aribstr -laribstr \
	-L$(ROOTDIR)/psisi_manager -lpsisimanager \
	-L$(ROOTDIR)/stream_handler -lstreamhandler \
	-lpthread \

USERDEFS	:= \


#
#   Target object
#
TARGET_NAME	:=	viewingmanager

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	ViewingManager.cpp \
	ViewingManagerCommands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
