#
#   Defines
#
BASEDIR		:=	../

INCLUDES	:= \
	-I./ \

LIBS		:= \
	-lpthread \
	-rdynamic \

USERDEFS	:= \
	-D_MULTI_REQUESTING \
	-D_REQUEST_TIMEOUT \
	-D_LOG_SIMPLE \

#
#   Target object
#
TARGET_NAME	:=	threadmgr

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS		:= \
	threadmgr.c \
	threadmgr_if.c \
	threadmgr_util.c \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
