#
#   Defines
#
BASEDIR		:=	../../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR) \
	-I$(ROOTDIR)/threadmgr \
	-I$(ROOTDIR)/threadmgrpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/psisi_manager \
	-I./ \

LIBS		:= \
	-L$(ROOTDIR)/threadmgr -lthreadmgr \
	-L$(ROOTDIR)/threadmgrpp -lthreadmgrpp \
	-L$(ROOTDIR)/common -lcommon \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \

#
#   Target object
#
TARGET_NAME	:=	tunerservice

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	TunerService.cpp \
	TunerServiceCommands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
