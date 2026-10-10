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
	-I../third_party/aribstr \

LIBS		:= \
	-L$(ROOTDIR)/common -lcommon \
	-L../third_party/aribstr -laribstr \


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
	program_association_table.cpp \
	program_map_table.cpp \
	time_offset_table.cpp \
	event_information_table.cpp \
	event_information_table_sched.cpp \
	network_information_table.cpp \
	service_description_table.cpp \
	running_status_table.cpp \
	broadcaster_information_table.cpp \
	conditional_access_table.cpp \
	common_data_table.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

