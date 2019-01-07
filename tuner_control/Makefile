#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR)/threadmgr \
	-I$(BASEDIR)/threadmgrpp \
	-I$(BASEDIR)/common \
	-I./ \
	-I./it9175 \
	-I../ \

LIBS		:= \
	-L$(BASEDIR)/threadmgr -lthreadmgr \
	-L$(BASEDIR)/threadmgrpp -lthreadmgrpp \
	-L$(BASEDIR)/common -lcommon \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \
	it9175 \
	it9175_test \


#
#   Target object
#
TARGET_NAME	:=	tuner_control

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


#
#   Configurations
#
include $(BASEDIR)/Config.mak
