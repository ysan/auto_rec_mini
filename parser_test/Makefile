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
	-I../parser \
	-I../parser/aribstr/ \
	-I../parser/psisi/ \
	-I../parser/psisi/descriptor \
	-I../parser/dsmcc/ \
	-I../parser/dsmcc/dsmcc_descriptor \
	-I/usr/include/PCSC/ \

LIBS		:= \
	-lpthread \
	-lpcsclite \
	-L../parser/aribstr -laribstr \
	-L../parser/psisi -lpsisiparser \
	-L../parser/dsmcc -ldsmccparser \
	-L../parser -lparser \
	-L../common -lcommon \

SUBDIRS		:= \

#
#   Target object
#
TARGET_NAME	:=	parser_test
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
	Parser_test.cpp \


#
#   Configurations
#
include $(BASEDIR)/Config.mak

