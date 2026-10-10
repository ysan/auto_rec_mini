#
#   Defines
#
BASEDIR		:=	../../../../

CFLAGS		:= \
	-std=c++11 \
	-rdynamic \
	-funwind-tables \

INCLUDES	:= \
	-I./ \
	-I../coseq \

LIBS		:= \
	-lpthread \
	-L../coseq -lcoseq \

#
#   Target object
#
TARGET_NAME	:=	coseqpp

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	SHARED

#
#   Compile sources
#
SRCS_CPP	:= \
	coseqpp_base.cpp \
	coseqpp.cpp \

#
#   Configurations
#
include $(BASEDIR)/Config.mak
