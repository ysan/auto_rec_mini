#
#   Defines
#
BASEDIR		:=	../../../../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11
INCLUDES	:= \
	-I$(ROOTDIR) \
	-I$(ROOTDIR)/common \
	-I./ \
	-I../ \
	-I../../ \
	-I../../aribstr \

LIBS		:= \
	-L../aribstr -laribstr \
	-L$(ROOTDIR)/common -lcommon \


#
#   Target object
#
TARGET_NAME	:=	dsmcc_descriptor

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	OBJECT

#
#   Compile sources
#
SRCS		:=

SRCS_CPP	:= \
	DsmccDescriptor.cpp \

#
#   Configurations
#
include $(BASEDIR)/Config.mak

