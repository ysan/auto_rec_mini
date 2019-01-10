#
#   Defines
#

MAKE		:=	/usr/bin/make --no-print-directory
#CC			:=	/bin/gcc
CC			:=	/usr/bin/gcc
CPP			:=	/usr/bin/g++
AR			:=	/usr/bin/ar
RANLIB		:=	/usr/bin/ranlib
MKDIR		:=	/bin/mkdir
RM			:=	/bin/rm
INSTALL		:=	/usr/bin/install
SIZE		:=	/usr/bin/size


CFLAGS		+=	-Wall -MD
ifeq ($(TARGET_TYPE), SHARED)
CFLAGS		+=	-shared -fPIC
else ifeq ($(TARGET_TYPE), STATIC)
CFLAGS		+=	-fPIC
else ifeq ($(TARGET_TYPE), OBJECT)
CFLAGS		+=	-fPIC
endif

ifneq ($(USERDEFS),)
CFLAGS		+=	$(USERDEFS)
endif

EXIST_SRCS		:=	FALSE
ifneq ($(SRCS),)
EXIST_SRCS		:=	TRUE
else ifneq ($(SRCS_CPP),)
EXIST_SRCS		:=	TRUE
endif

ifeq ($(EXIST_SRCS), TRUE)
OBJDIR		:=	./objs/
OBJS		:=	$(SRCS:%.c=$(OBJDIR)/%.o)
OBJS		+=	$(SRCS_CPP:%.cpp=$(OBJDIR)/%.o)
DEPENDS		:=	$(OBJS:%.o=%.d)
endif

ifneq ($(TARGET_NAME),)
ifeq ($(TARGET_TYPE), EXEC)
TARGET_OBJ	:=	$(TARGET_NAME)

else ifeq ($(TARGET_TYPE), SHARED)
TARGET_OBJ	:=	$(addprefix lib, $(addsuffix .so, $(TARGET_NAME)))

else ifeq ($(TARGET_TYPE), STATIC)
TARGET_OBJ	:=	$(addprefix lib, $(addsuffix .a, $(TARGET_NAME)))

else ifeq ($(TARGET_TYPE), OBJECT)
TARGET_OBJ	:=	$(addsuffix .o, $(TARGET_NAME))

else
TARGET_OBJ	:=	dummy
endif
endif


INSTALLDIR		?=	$(BASEDIR)
ifneq ($(INSTALLDIR),)
INSTALLDIR_BIN	:=	$(INSTALLDIR)/bin
INSTALLDIR_LIB	:=	$(INSTALLDIR)/lib
endif


#
#   Recipes
#
ifneq ($(TARGET_OBJ),)

# TARGET_TYPE = EXEC -------------------------------------
ifeq ($(TARGET_TYPE), EXEC)
#all: pre_proc subdirs target post_proc
all:
	@$(MAKE) subdirs
	@$(MAKE) pre_proc
	@$(MAKE) target
	@$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
ifneq ($(SRCS_CPP),)
	$(CPP) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
else
	$(CC) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
endif
	@echo
	@$(SIZE) $(TARGET_OBJ)
	@echo

$(OBJDIR)/%.o: %.c
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

# TARGET_TYPE = SHARED -----------------------------------
else ifeq ($(TARGET_TYPE), SHARED)
#all: pre_proc subdirs target post_proc
all:
	@$(MAKE) subdirs
	@$(MAKE) pre_proc
	@$(MAKE) target
	@$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
ifneq ($(SRCS_CPP),)
	$(CPP) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
else
	$(CC) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
endif
	@echo
	@$(SIZE) $(TARGET_OBJ)
	@echo

$(OBJDIR)/%.o: %.c
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

# TARGET_TYPE = STATIC -----------------------------------
else ifeq ($(TARGET_TYPE), STATIC)
#all: pre_proc subdirs target post_proc
all:
	@$(MAKE) subdirs
	@$(MAKE) pre_proc
	@$(MAKE) target
	@$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
	$(AR) rv $@ $^
	$(RANLIB) $@
	@echo
	@$(SIZE) $(TARGET_OBJ)
	@echo

$(OBJDIR)/%.o: %.c
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

# TARGET_TYPE = OBJECT -----------------------------------
else ifeq ($(TARGET_TYPE), OBJECT)
#all: pre_proc subdirs target post_proc
all:
	@$(MAKE) subdirs
	@$(MAKE) pre_proc
	@$(MAKE) target
	@$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
	$(AR) rvs $@ $^
	@echo
	@$(SIZE) $(TARGET_OBJ)
	@echo

$(OBJDIR)/%.o: %.c
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

#---------------------------------------------------------
else
all:
	@echo "TARGET_TYPE is invalid."
	@echo "  dir:[$(PWD)]"
	exit 1
endif

else # not define TARGET_OBJ
all:
	@$(MAKE) subdirs
endif


pre_proc:
	@echo "======="
	@echo "=======  dir:[$(PWD)]"
	@echo "======="

post_proc:
	@echo


clean:
	@$(MAKE) clean-r
ifneq ($(INSTALLDIR_BIN),)
	$(RM) -rf $(INSTALLDIR_BIN)
endif
ifneq ($(INSTALLDIR_LIB),)
	$(RM) -rf $(INSTALLDIR_LIB)
endif

clean-r:
	@$(MAKE) clean-subdirs
	@$(MAKE) pre_proc
ifneq ($(TARGET_OBJ),)
	$(RM) -f $(TARGET_OBJ)
endif
ifneq ($(OBJS),)
	$(RM) -f $(OBJS)
endif
ifneq ($(DEPENDS),)
	$(RM) -f $(DEPENDS)
endif
	@$(MAKE) post_proc


install:
	@$(MAKE) all
	@$(MAKE) install-r

install-r:
	@$(MAKE) install-subdirs
	@$(MAKE) pre_proc
ifeq ($(TARGET_TYPE), EXEC)
ifneq ($(TARGET_OBJ),)
ifneq ($(INSTALLDIR_BIN),)
	@$(INSTALL) -v -d $(INSTALLDIR_BIN)
	@$(INSTALL) -v $(TARGET_OBJ) $(INSTALLDIR_BIN)
endif
endif
else ifeq ($(TARGET_TYPE), SHARED)
ifneq ($(TARGET_OBJ),)
ifneq ($(INSTALLDIR_LIB),)
	@$(INSTALL) -v -d $(INSTALLDIR_LIB)
	@$(INSTALL) -v $(TARGET_OBJ) $(INSTALLDIR_LIB)
endif
endif
endif
	@$(MAKE) post_proc


subdirs:
ifneq ($(SUBDIRS),)
	@for d in $(SUBDIRS) ; do \
		(cd $$d && $(MAKE)) || exit 1 ;\
	done
else
	@echo -n "" # dummy
endif

clean-subdirs:
ifneq ($(SUBDIRS),)
	@for d in $(SUBDIRS) ; do \
		(cd $$d && $(MAKE) clean-r) || exit 1 ;\
	done
else
	@echo -n "" # dummy
endif

install-subdirs:
ifneq ($(SUBDIRS),)
	@for d in $(SUBDIRS) ; do \
		(cd $$d && $(MAKE) install-r) || exit 1 ;\
	done
else
	@echo -n "" # dummy
endif



#
#   Phony defines
#
.PHONY: all target clean clean-r install install-r subdirs clean-subdirs install-subdirs pre_proc post_proc


#
#   Include dependency-files
#
ifneq ($(DEPENDS),)
-include $(DEPENDS)
endif

