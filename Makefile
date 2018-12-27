#
#   Defines
#
BASEDIR		:=	./

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I./ \
	-I$(BASEDIR)/aribstr/ \
	-I$(BASEDIR)/psisi/ \
	-I$(BASEDIR)/psisi/descriptor \
	-I$(BASEDIR)/dsmcc/ \
	-I$(BASEDIR)/dsmcc/dsmcc_descriptor \
	-I/usr/include/PCSC/ \

LIBS		:= \
	-lpthread \
	-lpcsclite \
	-L$(BASEDIR)/aribstr -laribstr \
	-L$(BASEDIR)/psisi -lpsisi \
	-L$(BASEDIR)/dsmcc -ldsmcc \

SUBDIRS		:= \
	aribstr \
	psisi \
	dsmcc \

#
#   Target object
#
TARGET_NAME	:=	atpp

#
#   Target type
#     (EXEC/SHARED/STATIC/OBJECT)
#
TARGET_TYPE	:=	EXEC

#
#   Compile sources
#
SRCS		:=

SRCS_CPP	:= \
	main.cpp \
	TsParser.cpp \
	SectionParser.cpp \
	Utils.cpp \
	TsCommon.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

