#
#   Defines
#
BASEDIR		:=	../

INCLUDES	:= \
	-I./ \
	-I../threadmgr \

LIBS		:= \
	-lpthread \
	-rdynamic \
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


#
#   Configurations
#
include $(BASEDIR)/Config.mak
