#
#   Defines
#
BASEDIR		:=	../../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR) \
	-I$(BASEDIR)/common \
	-I./ \
	-I./dsmcc_descriptor \
	-I../ \

LIBS		:= \
	-L../aribstr -laribstr \
	-L$(BASEDIR)/common -lcommon \


APPEND_OBJS := ./dsmcc_descriptor/dsmcc_descriptor.o

SUBDIRS		:= ./dsmcc_descriptor

#
#   Target object
#
TARGET_NAME	:=	dsmccparser

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

