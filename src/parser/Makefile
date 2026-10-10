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
	-I./third_party/aribstr/ \
	-I./third_party/tssplitter_lite/ \
	-I./psisi/ \
	-I./psisi/descriptor \
	-I./dsmcc/ \
	-I./dsmcc/dsmcc_descriptor \
	-I/usr/include/PCSC/ \

LIBS		:= \
	-L$(ROOTDIR)/common -lcommon \
	-L./third_party/aribstr -laribstr \
	-L./third_party/tssplitter_lite -ltssplitter_lite \
	-L./psisi -lpsisiparser \
	-L./dsmcc -ldsmccparser \
	-lpthread \
	-lpcsclite \

SUBDIRS		:= \
	third_party/aribstr \
	third_party/tssplitter_lite \
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
	ts_parser.cpp \
	section_parser.cpp \
	ts_arib_common.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

