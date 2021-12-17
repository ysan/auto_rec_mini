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
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/command_server \
	-I./ \
	-I./tune_thread \

LIBS		:= \
	-L$(ROOTDIR)/threadmgr -lthreadmgr \
	-L$(ROOTDIR)/threadmgrpp -lthreadmgrpp \
	-L$(ROOTDIR)/common -lcommon \
	-L./tune_thread -ltunethread \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \
	recfsusb2i \
	recdvb \
	tune_thread \


#
#   Target object
#
TARGET_NAME	:=	tunercontrol

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	TunerControl.cpp \
	TunerControlCommands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
