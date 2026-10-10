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
	-I../../third_party/aribstr \

LIBS		:= \
	-L../third_party/aribstr -laribstr \
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
	dsmcc_descriptor.cpp \

#
#   Configurations
#
include $(BASEDIR)/Config.mak

