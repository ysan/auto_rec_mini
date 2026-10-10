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
	-L$(ROOTDIR)/common -lcommon \
	-L./third_party/aribstr -laribstr \


#
#   Target object
#
TARGET_NAME	:=	descriptor

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
	descriptor.cpp \
	short_event_descriptor.cpp \
	extended_event_descriptor.cpp \
	component_descriptor.cpp \
	audio_component_descriptor.cpp \
	series_descriptor.cpp \
	content_descriptor.cpp \
	service_list_descriptor.cpp \
	satellite_delivery_system_descriptor.cpp \
	terrestrial_delivery_system_descriptor.cpp \
	ts_information_descriptor.cpp \
	partial_reception_descriptor.cpp \
	service_descriptor.cpp \
	si_parameter_descriptor.cpp \
	network_name_descriptor.cpp \
	ca_identifier_descriptor.cpp \
	conditional_access_descriptor.cpp \
	stream_identifier_descriptor.cpp \
	logo_transmission_descriptor.cpp \

#
#   Configurations
#
include $(BASEDIR)/Config.mak

