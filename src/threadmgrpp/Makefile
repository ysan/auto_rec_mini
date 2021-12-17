#
#   Defines
#
BASEDIR		:=	../../

CFLAGS		:= \
	-std=c++11 \
	-rdynamic \
	-funwind-tables \

INCLUDES	:= \
	-I./ \
	-I../threadmgr \

LIBS		:= \
	-lpthread \
	-L../threadmgr -lthreadmgr \

USERDEFS	:= \
#	-D_LOG_SIMPLE \

#
#   Target object
#
TARGET_NAME	:=	threadmgrpp

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	ThreadMgrIf.cpp \
	ThreadMgrExternalIf.cpp \
	ThreadMgrBase.cpp \
	ThreadMgr.cpp \

#INSTALL_HEADERS := \
#	ThreadMgrIf.h \
#	ThreadMgrExternalIf.h \
#	ThreadMgrBase.h \
#	ThreadMgr.h \
#	ThreadMgrpp.h \

#
#   Configurations
#
include $(BASEDIR)/Config.mak
