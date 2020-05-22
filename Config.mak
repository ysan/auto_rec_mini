#
#   Defines
#

MAKE		:=	/usr/bin/make
ECHO		:=	echo
ifeq ($(WITH_CLANG), 1)
CC			:=	/usr/bin/clang
CPP			:=	/usr/bin/clang++
else
CC			:=	/usr/bin/gcc
CPP			:=	/usr/bin/g++
endif
AR			:=	/usr/bin/ar
RANLIB		:=	/usr/bin/ranlib
MKDIR		:=	/bin/mkdir
RMDIR		:=	/bin/rmdir
RM			:=	/bin/rm
INSTALL		:=	/usr/bin/install
LDD			:=	/usr/bin/ldd
SIZE		:=	/usr/bin/size
OBJDUMP		:=	/usr/bin/objdump
GCOV		:=	/usr/bin/gcov

MAKE_OPTIONS	:=	--no-print-directory


# add user define
-include $(BASEDIR)/user.rules


# redefine options for recursive MAKE
ifeq ($(NO_STRIP), 1)
MAKE_OPTIONS	+=	NO_STRIP=1
endif
MAKE		+=	$(MAKE_OPTIONS)


# CFLAGS
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

ifeq ($(WITH_ASAN), 1)
CFLAGS		+=	-fsanitize=address -fno-omit-frame-pointer
endif

ifeq ($(WITH_COVERAGE), 1)
CFLAGS		+=	--coverage
endif


# EXIST_SRCS
EXIST_SRCS		:=	FALSE
ifneq ($(SRCS),)
EXIST_SRCS		:=	TRUE
else ifneq ($(SRCS_CPP),)
EXIST_SRCS		:=	TRUE
endif


# OBJS/etc
ifeq ($(EXIST_SRCS), TRUE)
OBJDIR		:=	./objs/
OBJS		:=	$(SRCS:%.c=$(OBJDIR)/%.o)
OBJS		+=	$(SRCS_CPP:%.cpp=$(OBJDIR)/%.o)
DEPENDS		:=	$(OBJS:%.o=%.d)
GCDAS		:=	$(OBJS:%.o=%.gcda)
GCNOS		:=	$(OBJS:%.o=%.gcno)
endif


# TARGET_OBJ
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


# INSTALLDIR
INSTALLDIR		?=	$(BASEDIR)/local_build
ifneq ($(INSTALLDIR),)
ifeq ($(NOT_INSTALL),)
INSTALLDIR_BIN	:=	$(INSTALLDIR)/bin
ifeq ($(INSTALLDIR_INC_BASE),)
INSTALLDIR_INC	:=	$(INSTALLDIR)/include
else
INSTALLDIR_INC	:=	$(INSTALLDIR)/include/$(INSTALLDIR_INC_BASE)
endif
ifeq ($(INSTALLDIR_LIB_BASE),)
INSTALLDIR_LIB	:=	$(INSTALLDIR)/lib
else
INSTALLDIR_LIB	:=	$(INSTALLDIR)/lib/$(INSTALLDIR_LIB_BASE)
endif
endif
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
ifeq ($(WITH_COVERAGE), 1)
	@$(MAKE) gcov
endif
	@$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
ifneq ($(SRCS_CPP),)
	$(CPP) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
else
	$(CC) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
endif
	@$(MAKE) post_proc_target

$(OBJDIR)/%.o: %.c
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

gcov:
	@for gcda in $(GCDAS) ; do \
		if [ -e $$gcda ]; then \
			$(ECHO) "found [$$gcda]";\
			$(GCOV) -r -o $(OBJDIR) $$gcda; \
		fi \
	done

# TARGET_TYPE = SHARED -----------------------------------
else ifeq ($(TARGET_TYPE), SHARED)
#all: pre_proc subdirs target post_proc
all:
	@$(MAKE) subdirs
	@$(MAKE) pre_proc
	@$(MAKE) target
ifeq ($(WITH_COVERAGE), 1)
	@$(MAKE) gcov
endif
	@$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
ifneq ($(SRCS_CPP),)
	$(CPP) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
else
	$(CC) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)
endif
	@$(MAKE) post_proc_target

$(OBJDIR)/%.o: %.c
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

gcov:
	@for gcda in $(GCDAS) ; do \
		if [ -e $$gcda ]; then \
			$(ECHO) "found [$$gcda]";\
			$(GCOV) -r -o $(OBJDIR) $$gcda; \
		fi \
	done

# TARGET_TYPE = STATIC -----------------------------------
else ifeq ($(TARGET_TYPE), STATIC)
#all: pre_proc subdirs target post_proc
all:
	@$(MAKE) subdirs
	@$(MAKE) pre_proc
	@$(MAKE) target
ifeq ($(WITH_COVERAGE), 1)
	@$(MAKE) gcov
endif
	@$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
	$(AR) rv $@ $^
	$(RANLIB) $@
	@$(MAKE) post_proc_target

$(OBJDIR)/%.o: %.c
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

gcov:
	@for gcda in $(GCDAS) ; do \
		if [ -e $$gcda ]; then \
			$(ECHO) "found [$$gcda]";\
			$(GCOV) -r -o $(OBJDIR) $$gcda; \
		fi \
	done

# TARGET_TYPE = OBJECT -----------------------------------
else ifeq ($(TARGET_TYPE), OBJECT)
#all: pre_proc subdirs target post_proc
all:
	@$(MAKE) subdirs
	@$(MAKE) pre_proc
	@$(MAKE) target
ifeq ($(WITH_COVERAGE), 1)
	@$(MAKE) gcov
endif
	@$(MAKE) post_proc

target: $(TARGET_OBJ)

$(TARGET_OBJ): $(OBJS) $(APPEND_OBJS)
	$(AR) rvs $@ $^
	@$(MAKE) post_proc_target

$(OBJDIR)/%.o: %.c
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJDIR)/%.o: %.cpp
#	@$(MKDIR) -p -m 775 $(OBJDIR)
	@$(MKDIR) -p -m 775 $(dir $@)
	$(CPP) -c $< -o $@ $(CFLAGS) $(INCLUDES)

gcov:
	@for gcda in $(GCDAS) ; do \
		if [ -e $$gcda ]; then \
			$(ECHO) "found [$$gcda]";\
			$(GCOV) -o $(OBJDIR) $$gcda; \
		fi \
	done

#---------------------------------------------------------
else
all:
	@$(ECHO) "TARGET_TYPE is invalid."
	@$(ECHO) "  dir:[$(PWD)]"
	exit 1
endif

else # not define TARGET_OBJ
all:
	@$(MAKE) subdirs
endif


pre_proc:
	@$(ECHO) "========================================"
	@$(ECHO) "-- $(PWD)"
	@$(ECHO) "========================================"

post_proc:
	@$(ECHO)

post_proc_target:
	@$(ECHO)
	@$(ECHO) "  ## $(TARGET_OBJ) ## created."
ifeq ($(TARGET_TYPE), EXEC)
	@$(OBJDUMP) -f $(TARGET_OBJ)
else ifeq ($(TARGET_TYPE), SHARED)
	@$(LDD) $(TARGET_OBJ)
else ifeq ($(TARGET_TYPE), STATIC)
	@$(AR) -t $(TARGET_OBJ)
else ifeq ($(TARGET_TYPE), OBJECT)
	@$(AR) -t $(TARGET_OBJ)
endif
#	@$(SIZE) $(TARGET_OBJ)
	@$(ECHO)


clean:
	@$(MAKE) clean-r
#ifneq ($(INSTALLDIR_BIN),)
#	$(RMDIR) $(INSTALLDIR_BIN)
#endif
#ifneq ($(INSTALLDIR_LIB),)
#	$(RMDIR) $(INSTALLDIR_LIB)
#endif

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
ifneq ($(GCDAS),)
	$(RM) -f $(GCDAS)
endif
ifneq ($(GCNOS),)
	$(RM) -f $(GCNOS)
endif
	$(RM) -f *.gcov

ifeq ($(TARGET_TYPE), EXEC)
ifneq ($(TARGET_OBJ),)
ifneq ($(INSTALLDIR_BIN),)
	$(RM) -f $(INSTALLDIR_BIN)/$(TARGET_OBJ)
endif
endif

else ifeq ($(TARGET_TYPE), SHARED)
ifneq ($(TARGET_OBJ),)
ifneq ($(INSTALLDIR_LIB),)
	$(RM) -f $(INSTALLDIR_LIB)/$(TARGET_OBJ)
endif
endif
ifneq ($(INSTALL_HEADERS),)
ifneq ($(INSTALLDIR_INC),)
	@for h in $(INSTALL_HEADERS) ; do \
		$(RM) -f $(INSTALLDIR_INC)/$$h ;\
	done
endif
endif
endif
	@$(MAKE) post_proc


install:
	@$(MAKE) all
	@$(MAKE) install-r

install-r:
	@$(MAKE) install-subdirs
#	@$(MAKE) pre_proc
ifeq ($(TARGET_TYPE), EXEC)
ifneq ($(TARGET_OBJ),)
ifneq ($(INSTALLDIR_BIN),)
	@$(INSTALL) -v -d $(INSTALLDIR_BIN)
ifeq ($(NO_STRIP), 1)
	@$(INSTALL) -v $(TARGET_OBJ) $(INSTALLDIR_BIN)
else
	@$(INSTALL) -v --strip $(TARGET_OBJ) $(INSTALLDIR_BIN)
endif
endif
endif
else ifeq ($(TARGET_TYPE), SHARED)
ifneq ($(TARGET_OBJ),)
ifneq ($(INSTALLDIR_LIB),)
	@$(INSTALL) -v -d $(INSTALLDIR_LIB)
ifeq ($(NO_STRIP), 1)
	@$(INSTALL) -v $(TARGET_OBJ) $(INSTALLDIR_LIB)
else
	@$(INSTALL) -v --strip $(TARGET_OBJ) $(INSTALLDIR_LIB)
endif
endif
endif
ifneq ($(INSTALL_HEADERS),)
ifneq ($(INSTALLDIR_INC),)
	@$(INSTALL) -v -d $(INSTALLDIR_INC)
	@$(INSTALL) -v --mode=644 $(INSTALL_HEADERS) $(INSTALLDIR_INC)
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
	@$(ECHO) -n "" # dummy
endif

clean-subdirs:
ifneq ($(SUBDIRS),)
	@for d in $(SUBDIRS) ; do \
		(cd $$d && $(MAKE) clean-r) || exit 1 ;\
	done
else
	@$(ECHO) -n "" # dummy
endif

install-subdirs:
ifneq ($(SUBDIRS),)
	@for d in $(SUBDIRS) ; do \
		(cd $$d && $(MAKE) install-r) || exit 1 ;\
	done
else
	@$(ECHO) -n "" # dummy
endif



#
#   Phony defines
#
.PHONY: all target clean clean-r install install-r subdirs clean-subdirs install-subdirs pre_proc post_proc post_proc_target gcov


#
#   Include dependency-files
#
ifneq ($(DEPENDS),)
-include $(DEPENDS)
endif

