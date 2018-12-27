#
#   Defines
#
BASEDIR		:=	../../
CFLAGS		:=	-std=c++11 -fPIC
INCLUDES	:= \
	-I./ \
	-I$(BASEDIR) \
	-I$(BASEDIR)/section_parser \
	-I$(BASEDIR)/aribstr \

LIBS		:= \
	-L$(BASEDIR)/section_parser -lsection_parser \
	-L$(BASEDIR)/aribstr -laribstr \


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

