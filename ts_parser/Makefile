#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR)/ \
	-I$(BASEDIR)/common \
	-I./ \
	-I./aribstr/ \
	-I./psisi/ \
	-I./psisi/descriptor \
	-I./dsmcc/ \
	-I./dsmcc/dsmcc_descriptor \
	-I/usr/include/PCSC/ \

LIBS		:= \
	-L$(BASEDIR)/common -lcommon \
	-L./aribstr -laribstr \
	-L./psisi -lpsisi \
	-L./dsmcc -ldsmcc \
	-lpthread \
	-lpcsclite \

SUBDIRS		:= \
	aribstr \
	psisi \
	dsmcc \

#
#   Target object
#
TARGET_NAME	:=	tsparser

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
	TsParser.cpp \
	SectionParser.cpp \
	TsAribCommon.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

