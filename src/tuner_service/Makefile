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
	-I$(ROOTDIR)/command_server \
	-I$(ROOTDIR)/tuner_control \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/psisi_manager \
	-I$(ROOTDIR)/channel_manager \
	-I./ \

LIBS		:= \
	-L$(ROOTDIR)/third_party/coseq/coseq -lcoseq \
	-L$(ROOTDIR)/third_party/coseq/coseqpp -lcoseqpp \
	-L$(ROOTDIR)/common -lcommon \
	-lpthread \

USERDEFS	:= \

SUBDIRS		:= \

#
#   Target object
#
TARGET_NAME	:=	tuner_service

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	tuner_service.cpp \
	tuner_service_commands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
