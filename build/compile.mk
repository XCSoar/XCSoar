######## tools

# August2111: This part is for OpenSoar only:
#============================================
# with changed .config a new ProgramVersion (and a new title.svg) is needed
output/include/ProgramVersion.h: OpenSoar.config
	@$(NQ)echo "  VERSION:   $< ==> $@ "
	$(Q)python3 $(topdir)/tools/python/replace.py $< Data/graphics/title.svg $(DATA)/temp/graphics/title.svg $@

# Version.o need the new PROGRAM_VERSION if it is available:
Version.o: %.cpp ProgramVersion.h
	@$(NQ)echo "  CPP     $@"
	$(Q)$(WRAPPED_CXX) $< -c -o $@ $(cxx-flags)
#================================================

CCACHE := 
ifeq ($(USE_CCACHE),y)
  CCACHE := ccache$(EXE)
endif

EXE := $(findstring .exe,$(MAKE))
AR = $(TCPREFIX)ar$(EXE)
RANLIB = $(TCPREFIX)ranlib$(EXE)

ifneq ($(ANALYZER),y)
  ifeq ($(CLANG),y)
    CXX = $(LLVM_PREFIX)clang++$(LLVM_SUFFIX)$(EXE)
    CC = $(LLVM_PREFIX)clang$(LLVM_SUFFIX)$(EXE)
  else
    CXX = $(TCPREFIX)g++$(TCSUFFIX)$(EXE)
    CC = $(TCPREFIX)gcc$(TCSUFFIX)$(EXE)
  endif
endif

ifeq ($(CLANG),y)
  AS = $(CC)
  ASFLAGS += -c -xassembler
  ifneq ($(LLVM_TARGET),)
    ASFLAGS += -target $(LLVM_TARGET)
  else
    ASFLAGS += $(TARGET_ARCH)
  endif

else
  AS = $(TCPREFIX)as$(EXE)
endif

LD = $(TCPREFIX)ld$(EXE)
DLLTOOL = $(TCPREFIX)dlltool$(EXE)
SIZE = $(TCPREFIX)size$(EXE)
STRIP = $(TCPREFIX)strip$(EXE)
WINDRES = $(TCPREFIX)windres$(EXE)
ARFLAGS = rcs

ifeq ($(CLANG)$(TARGET_IS_DARWIN)$(LTO),nny)
# use gcc's "ar" wrapper which takes care for loading the LTO plugin
AR = $(TCPREFIX)gcc-ar$(TCSUFFIX)$(EXE)
endif

ifeq ($(CLANG)$(TARGET_IS_DARWIN)$(LTO),yny)
AR = $(LLVM_PREFIX)llvm-ar$(LLVM_SUFFIX)$(EXE)
# ranlib has nothing to do for LLVM bytecode which is emitted in Clang's LTO
# mode. Use "true" command as dummy.
RANLIB = true
endif

CXX_VERSION = $(shell $(CXX) -dumpversion)
CXX_MAJOR_VERSION = $(firstword $(subst ., ,$(CXX_VERSION)))

####### paths

OBJ_SUFFIX = .o

# Converts a list of source file names to *.o
SRC_TO_OBJ = $(subst /./,/,$(patsubst %.cpp,%$(OBJ_SUFFIX),$(patsubst %.cxx,%$(OBJ_SUFFIX),$(patsubst %.c,%$(OBJ_SUFFIX),$(addprefix $(ABI_OUTPUT_DIR)/,$(1))))))

####### dependency handling

DEPFILE = $(@:$(OBJ_SUFFIX)=.d)
DEPFLAGS = -Wp,-MD,$(DEPFILE),-MT,$@
cc-flags = $(DEPFLAGS) $(ALL_CFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) $(FLAGS_COVERAGE)  $(EXTRA_CPPFLAGS)  $(EXTRA_CFLAGS)
cxx-flags = $(DEPFLAGS) $(ALL_CXXFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) $(FLAGS_COVERAGE)  $(EXTRA_CPPFLAGS)  $(EXTRA_CXXFLAGS)

#
# Useful debugging targets - make preprocessed versions of the source
#
$(ABI_OUTPUT_DIR)/%.i: %.cpp FORCE
	$(CXX) $< -E -o $@ $(cxx-flags)

$(ABI_OUTPUT_DIR)/%.s: %.cpp FORCE
	$(CXX) $< -S -o $@ $(cxx-flags)

$(ABI_OUTPUT_DIR)/%.i: %.cxx FORCE
	$(CXX) $< -E -o $@ $(cxx-flags)

$(ABI_OUTPUT_DIR)/%.s: %.cxx FORCE
	$(CXX) $< -S -o $@ $(cxx-flags)

$(ABI_OUTPUT_DIR)/%.i: %.c FORCE
	$(CC) $< -E -o $@ $(cc-flags)

####### build rules
#
#
# Provide our own rules for building...
#

WRAPPED_CC = $(strip $(CCACHE) $(CC))
WRAPPED_CXX = $(strip $(CCACHE) $(CXX))

$(ABI_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.c | $(ABI_OUTPUT_DIR)/%/../dirstamp $(compile-depends)
	@$(NQ)echo "  CC      $@"
	$(Q)$(WRAPPED_CC) $< -c -o $@ $(cc-flags)

# switch verbose on (or off):
# cxx-flags += -v

$(ABI_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.cpp | $(ABI_OUTPUT_DIR)/%/../dirstamp $(compile-depends)
	@$(NQ)echo "  CPP     $@"
	$(Q)$(WRAPPED_CXX) $< -c -o $@ $(cxx-flags)
ifeq ($(IWYU),y)
	$(Q)iwyu $< $(cxx-flags)
endif

$(ABI_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.cxx | $(ABI_OUTPUT_DIR)/%/../dirstamp $(compile-depends)
	@$(NQ)echo "  CXX     $@"
	$(Q)$(WRAPPED_CXX) $< -c -o $@ $(cxx-flags)
ifeq ($(IWYU),y)
	$(Q)iwyu $< $(cxx-flags)
endif

# Note: $(compile-depends) contains a list of order-only targets which
# must be finished before anything can be compiled.  It can be used to
# prepare preprocessor includes.
