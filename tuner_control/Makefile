#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR) \
	-I$(BASEDIR)/threadmgr \
	-I$(BASEDIR)/threadmgrpp \
	-I$(BASEDIR)/common \
	-I$(BASEDIR)/parser \
	-I$(BASEDIR)/command_server \
	-I./ \
	-I./it9175 \
	-I./tune_thread \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-L./it9175 -lit9175 \
	-L./tune_thread -ltunethread \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \
	it9175 \
	it9175_test \
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
