#
#   Defines
#
BASEDIR		:=	../../../
ROOTDIR		:=	$(BASEDIR)/src

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR) \
	-I$(ROOTDIR)/third_party/coseq/coseq \
	-I$(ROOTDIR)/third_party/coseq/coseqpp \
	-I$(ROOTDIR)/common \
	-I$(ROOTDIR)/parser \
	-I./ \
	-I../ \

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
TARGET_NAME	:=	tune_thread

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	tune_thread.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak
