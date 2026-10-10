#
#   Defines
#
BASEDIR		:=	../../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR) \
	-I$(ROOTDIR)/third_party/coseq/coseq \
	-I$(ROOTDIR)/third_party/coseq/coseqpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/command_server \
	-I./ \
	-I./tune_thread \

LIBS		:= \
	-L$(ROOTDIR)/third_party/coseq/coseq -lcoseq \
	-L$(ROOTDIR)/third_party/coseq/coseqpp -lcoseqpp \
	-L$(ROOTDIR)/common -lcommon \
	-L./tune_thread -ltune_thread \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \
	recfsusb2i \
	recdvb \
	tune_thread \


#
#   Target object
#
TARGET_NAME	:=	tuner_control

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	tuner_control.cpp \
	tuner_control_commands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
