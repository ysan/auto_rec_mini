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
	-L../ts_parser/psisi -lpsisiparser \
	-L../ts_parser/dsmcc -ldsmccparser \
	-L../ts_parser -ltsparser \
	-L../common -lcommon \

SUBDIRS		:= \

#
#   Target object
#
TARGET_NAME	:=	tsparser_test
NOT_INSTALL :=	YES

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
	TsParser_test.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

