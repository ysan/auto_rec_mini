#
#   Defines
#
BASEDIR		:=	../

INCLUDES	:= \
	-I./ \

CFLAGS		:= \
	-rdynamic \
	-funwind-tables \

LIBS		:= \
	-lpthread \

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

#INSTALL_HEADERS := \
#	threadmgr_if.h \
#	threadmgr_util.h \

#
#   Configurations
#
include $(BASEDIR)/Config.mak
