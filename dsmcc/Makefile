#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I./ \
	-I./descriptor \
	-I$(BASEDIR) \

LIBS		:= \
	-L$(BASEDIR)/aribstr -laribstr \


APPEND_OBJS := ./descriptor/descriptor.o

SUBDIRS		:= ./descriptor

#
#   Target object
#
TARGET_NAME	:=	dsmcc

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS		:=

SRCS_CPP	:= \
	Dsmcc.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

