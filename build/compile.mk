######## tools

CCACHE := 
ifeq ($(USE_CCACHE),y)
  CCACHE := ccache$(EXE)
endif

EXE := $(findstring .exe,$(MAKE))
AR = $(TCPREFIX)ar$(EXE)
AS = $(TCPREFIX)as$(EXE)
ifneq ($(ANALYZER),y)
  ifeq ($(CLANG),y)
    CXX = $(TCPREFIX)clang++$(TCSUFFIX)$(EXE)
    CC = $(TCPREFIX)clang$(TCSUFFIX)$(EXE)
  else
    CXX = $(TCPREFIX)g++$(TCSUFFIX)$(EXE)
    CC = $(TCPREFIX)gcc$(TCSUFFIX)$(EXE)
  endif
endif
DLLTOOL = $(TCPREFIX)dlltool$(EXE)
SIZE = $(TCPREFIX)size$(EXE)
STRIP = $(TCPREFIX)strip$(EXE)
WINDRES = $(TCPREFIX)windres$(EXE)
ARFLAGS = rcs

ifeq ($(TARGET),WINE)
AR = ar$(EXE)
STRIP = strip$(EXE)
WINDRES = wrc$(EXE)
endif

ifeq ($(TARGET)$(CLANG),ANDROIDy)
# on NDK r8c, binutils are only in the gcc toolchain directory
AR = $(ANDROID_GCC_TOOLCHAIN)/bin/$(ANDROID_ABI4)-ar$(EXE)
STRIP = $(ANDROID_GCC_TOOLCHAIN)/bin/$(ANDROID_ABI4)-strip$(EXE)
endif

####### paths

ifeq ($(LLVM),y)
# generate LLVM bitcode
OBJ_SUFFIX = .bc
else
OBJ_SUFFIX = .o
endif

# Converts a list of source file names to *.o
SRC_TO_OBJ = $(patsubst %.cpp,%$(OBJ_SUFFIX),$(patsubst %.c,%$(OBJ_SUFFIX),$(addprefix $(TARGET_OUTPUT_DIR)/,$(1))))

####### dependency handling

DEPFILE = $(@:$(OBJ_SUFFIX)=.d)
DEPFLAGS = -Wp,-MD,$(DEPFILE),-MT,$@
cc-flags = $(DEPFLAGS) $(ALL_CFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) $(FLAGS_COVERAGE)
cxx-flags = $(DEPFLAGS) $(ALL_CXXFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) $(FLAGS_COVERAGE)

#
# Useful debugging targets - make preprocessed versions of the source
#
$(TARGET_OUTPUT_DIR)/%.i: %.cpp FORCE
	$(CXX) $(cxx-flags) -E $(OUTPUT_OPTION) $<

$(TARGET_OUTPUT_DIR)/%.s: %.cpp FORCE
	$(CXX) $(cxx-flags) -S $(OUTPUT_OPTION) $<

$(TARGET_OUTPUT_DIR)/%.i: %.c FORCE
	$(CC) $(cc-flags) -E $(OUTPUT_OPTION) $<

####### build rules
#
#
# Provide our own rules for building...
#

WRAPPED_CC = $(CCACHE) $(CC)
WRAPPED_CXX = $(CCACHE) $(CXX)

$(TARGET_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.c $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  CC      $@"
	$(Q)$(WRAPPED_CC) -c -o $@ $(cc-flags) $<

$(TARGET_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.cpp $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(WRAPPED_CXX) -c -o $@ $(cxx-flags) $<
