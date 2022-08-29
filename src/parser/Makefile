#
#   Defines
#
BASEDIR		:=	../../
ROOTDIR		:=	$(BASEDIR)/src/

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(ROOTDIR)/ \
	-I$(ROOTDIR)/common \
	-I./ \
	-I./aribstr/ \
	-I./tssplitter_lite/ \
	-I./psisi/ \
	-I./psisi/descriptor \
	-I./dsmcc/ \
	-I./dsmcc/dsmcc_descriptor \
	-I/usr/include/PCSC/ \

LIBS		:= \
	-L$(ROOTDIR)/common -lcommon \
	-L./aribstr -laribstr \
	-L./tssplitter_lite -ltssplitter_lite \
	-L./psisi -lpsisiparser \
	-L./dsmcc -ldsmccparser \
	-lpthread \
	-lpcsclite \

SUBDIRS		:= \
	aribstr \
	tssplitter_lite \
	psisi \
	dsmcc \

#
#   Target object
#
TARGET_NAME	:=	parser

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

