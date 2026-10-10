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
	-I$(ROOTDIR)/tuner_service \
	-I$(ROOTDIR)/parser \
	-I$(ROOTDIR)/psisi_manager \
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
TARGET_NAME	:=	channel_manager

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	channel_manager.cpp \
	channel_manager_commands.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
