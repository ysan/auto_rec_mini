#
#   Defines
#
BASEDIR		:=	../

CFLAGS		:=	-std=c++11

INCLUDES	:= \
	-I$(BASEDIR)/ \
	-I./ \
	-I../ \
	-I../common \
	-I../ts_parser \
	-I../ts_parser/aribstr/ \
	-I../ts_parser/psisi/ \
	-I../ts_parser/psisi/descriptor \
	-I../ts_parser/dsmcc/ \
	-I../ts_parser/dsmcc/dsmcc_descriptor \
	-I/usr/include/PCSC/ \

LIBS		:= \
	-lpthread \
	-lpcsclite \
	-L../ts_parser/aribstr -laribstr \
	-L../ts_parser/psisi -lpsisi \
	-L../ts_parser/dsmcc -ldsmcc \
	-L../ts_parser -lts_parser \
	-L../common -lcommon \

SUBDIRS		:= \

#
#   Target object
#
TARGET_NAME	:=	ts_parser

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


#
#   Configurations
#
include $(BASEDIR)/Config.mak

