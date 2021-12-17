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
	-I./descriptor \
	-I../ \
	-I../aribstr \

LIBS		:= \
	-L$(ROOTDIR)/common -lcommon \
	-L../aribstr -laribstr \


APPEND_OBJS := ./descriptor/descriptor.o

SUBDIRS		:= ./descriptor

#
#   Target object
#
TARGET_NAME	:=	psisiparser

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
	ProgramAssociationTable.cpp \
	ProgramMapTable.cpp \
	TimeOffsetTable.cpp \
	EventInformationTable.cpp \
	EventInformationTable_sched.cpp \
	NetworkInformationTable.cpp \
	ServiceDescriptionTable.cpp \
	RunningStatusTable.cpp \
	BroadcasterInformationTable.cpp \
	ConditionalAccessTable.cpp \
	CommonDataTable.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

