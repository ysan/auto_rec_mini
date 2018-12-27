#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I./ \
	-I./dsmcc_descriptor \
	-I$(BASEDIR) \

LIBS		:= \
	-L$(BASEDIR)/aribstr -laribstr \


APPEND_OBJS := ./dsmcc_descriptor/dsmcc_descriptor.o

SUBDIRS		:= ./dsmcc_descriptor

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

