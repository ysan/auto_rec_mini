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
	-I../../aribstr \

LIBS		:= \
	-L$(ROOTDIR)/common -lcommon \
	-L./aribstr -laribstr \


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
	Descriptor.cpp \
	ShortEventDescriptor.cpp \
	ExtendedEventDescriptor.cpp \
	ComponentDescriptor.cpp \
	AudioComponentDescriptor.cpp \
	SeriesDescriptor.cpp \
	ContentDescriptor.cpp \
	ServiceListDescriptor.cpp \
	SatelliteDeliverySystemDescriptor.cpp \
	TerrestrialDeliverySystemDescriptor.cpp \
	TSInformationDescriptor.cpp \
	PartialReceptionDescriptor.cpp \
	ServiceDescriptor.cpp \
	SIParameterDescriptor.cpp \
	NetworkNameDescriptor.cpp \
	CAIdentifierDescriptor.cpp \
	ConditionalAccessDescriptor.cpp \
	StreamIdentifierDescriptor.cpp \
	LogoTransmissionDescriptor.cpp \

#
#   Configurations
#
include $(BASEDIR)/Config.mak

