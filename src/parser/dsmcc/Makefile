#
#   Defines
#
BASEDIR		:=	../../../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR) \
	-I$(ROOTDIR)/common \
	-I./ \
	-I./dsmcc_descriptor \
	-I../ \

LIBS		:= \
	-L../aribstr -laribstr \
	-L$(ROOTDIR)/common -lcommon \


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

