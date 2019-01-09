#
#   Defines
#

#CC			:=	/bin/gcc
CC			:=	/usr/bin/gcc
CPP			:=	/usr/bin/g++
AR			:=	/usr/bin/ar
RANLIB		:=	/usr/bin/ranlib
MKDIR		:=	/bin/mkdir
RM			:=	/bin/rm
INSTALL		:=	/usr/bin/install


CFLAGS		+=	-Wall -MD -g
ifeq ($(TARGET_TYPE), SHARED)
CFLAGS		+=	-shared -fPIC
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
TARGET_OBJ	:=	invalid
endif
endif


INSTALLDIR		=	$(BASEDIR)
INSTALLDIR_BIN	:=	$(INSTALLDIR)/bin
INSTALLDIR_LIB	:=	$(INSTALLDIR)/lib


#
#   Recipes
#
ifneq ($(TARGET_OBJ),)

# TARGET_TYPE = EXEC -------------------------------------
ifeq ($(TARGET_TYPE), EXEC)
all: pre_proc subdirs target post_proc
#	$(MAKE) pre_proc
#	$(MAKE) subdirs
#	$(MAKE) target
#	$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
ifneq ($(SRCS_CPP),)
	$(CPP) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
else
	$(CC) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
endif

$(OBJDIR)/%.o: %.c
#	$(MKDIR) -p -m 775 $(OBJDIR)
	$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	$(MKDIR) -p -m 775 $(OBJDIR)
	$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

# TARGET_TYPE = SHARED -----------------------------------
else ifeq ($(TARGET_TYPE), SHARED)
all: pre_proc subdirs target post_proc
#	$(MAKE) pre_proc
#	$(MAKE) subdirs
#	$(MAKE) target
#	$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
ifneq ($(SRCS_CPP),)
	$(CPP) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
else
	$(CC) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
endif

$(OBJDIR)/%.o: %.c
#	$(MKDIR) -p -m 775 $(OBJDIR)
	$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	$(MKDIR) -p -m 775 $(OBJDIR)
	$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

# TARGET_TYPE = STATIC -----------------------------------
else ifeq ($(TARGET_TYPE), STATIC)
all: pre_proc subdirs target post_proc
#	$(MAKE) pre_proc
#	$(MAKE) subdirs
#	$(MAKE) target
#	$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
	$(AR) rv $@ $^
	$(RANLIB) $@

$(OBJDIR)/%.o: %.c
#	$(MKDIR) -p -m 775 $(OBJDIR)
	$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	$(MKDIR) -p -m 775 $(OBJDIR)
	$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

# TARGET_TYPE = OBJECT -----------------------------------
else ifeq ($(TARGET_TYPE), OBJECT)
all: pre_proc subdirs target post_proc
#	$(MAKE) pre_proc
#	$(MAKE) subdirs
#	$(MAKE) target
#	$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
	$(AR) rvs $@ $^

$(OBJDIR)/%.o: %.c
#	$(MKDIR) -p -m 775 $(OBJDIR)
	$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	$(MKDIR) -p -m 775 $(OBJDIR)
	$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

#---------------------------------------------------------
else
all:
	@echo "TARGET_TYPE is invalid."
	@echo "  current directory => [$(PWD)]"
	exit 1
endif

else # not define TARGET_OBJ
all:
#	@echo "TARGET_OBJ is not defined."
	$(MAKE) subdirs
endif


pre_proc:

post_proc:


clean:
	$(MAKE) clean-r
ifneq ($(INSTALLDIR_BIN),)
	$(RM) -rf $(INSTALLDIR_BIN)
endif
ifneq ($(INSTALLDIR_LIB),)
	$(RM) -rf $(INSTALLDIR_LIB)
endif

clean-r:
	$(MAKE) clean-subdirs
ifneq ($(TARGET_OBJ),)
	$(RM) -f $(TARGET_OBJ)
endif
ifneq ($(OBJS),)
	$(RM) -f $(OBJS)
endif
ifneq ($(DEPENDS),)
	$(RM) -f $(DEPENDS)
endif

install: all
	$(MAKE) install-subdirs
ifeq ($(TARGET_TYPE), EXEC)
ifneq ($(TARGET_OBJ),)
ifneq ($(INSTALLDIR_BIN),)
	$(INSTALL) -v -d $(INSTALLDIR_BIN)
	$(INSTALL) -v $(TARGET_OBJ) $(INSTALLDIR_BIN)
endif
endif
else ifeq ($(TARGET_TYPE), SHARED)
ifneq ($(TARGET_OBJ),)
ifneq ($(INSTALLDIR_LIB),)
	$(INSTALL) -v -d $(INSTALLDIR_LIB)
	$(INSTALL) -v $(TARGET_OBJ) $(INSTALLDIR_LIB)
endif
endif
endif


subdirs:
ifneq ($(SUBDIRS),)
	@for d in $(SUBDIRS) ; do \
		(cd $$d && $(MAKE)) || exit 1 ;\
	done
endif

clean-subdirs:
ifneq ($(SUBDIRS),)
	@for d in $(SUBDIRS) ; do \
		(cd $$d && $(MAKE) clean) || exit 1 ;\
	done
endif

install-subdirs:
ifneq ($(SUBDIRS),)
	@for d in $(SUBDIRS) ; do \
		(cd $$d && $(MAKE) install) || exit 1 ;\
	done
endif



#
#   Phony defines
#
.PHONY: all clean clean-r install subdirs clean-subdirs install-subdirs pre_proc post_proc


#
#   Include dependency-files
#
ifneq ($(DEPENDS),)
-include $(DEPENDS)
endif

