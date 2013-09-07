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
    CXX = $(LLVM_PREFIX)clang++$(LLVM_SUFFIX)$(EXE)
    CC = $(LLVM_PREFIX)clang$(LLVM_SUFFIX)$(EXE)
  else
    CXX = $(TCPREFIX)g++$(TCSUFFIX)$(EXE)
    CC = $(TCPREFIX)gcc$(TCSUFFIX)$(EXE)
  endif
endif
DLLTOOL = $(TCPREFIX)dlltool$(EXE)
SIZE = $(TCPREFIX)size$(EXE)
STRIP = $(TCPREFIX)strip$(EXE)
WINDRES = $(TCPREFIX)windres$(EXE)
ARFLAGS = -rcs

ifeq ($(TARGET),WINE)
AR = ar$(EXE)
STRIP = strip$(EXE)
WINDRES = wrc$(EXE)
endif

####### paths

ifeq ($(LLVM),y)
# generate LLVM bitcode
OBJ_SUFFIX = .bc
else
OBJ_SUFFIX = .o
endif

# Converts a list of source file names to *.o
SRC_TO_OBJ = $(subst /./,/,$(patsubst %.cpp,%$(OBJ_SUFFIX),$(patsubst %.c,%$(OBJ_SUFFIX),$(addprefix $(TARGET_OUTPUT_DIR)/,$(1)))))

####### dependency handling

DEPFILE = $(@:$(OBJ_SUFFIX)=.d)
DEPFLAGS = -Wp,-MD,$(DEPFILE),-MT,$@
cc-flags = $(DEPFLAGS) $(ALL_CFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) $(FLAGS_COVERAGE)
cxx-flags = $(DEPFLAGS) $(ALL_CXXFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) $(FLAGS_COVERAGE)

#
# Useful debugging targets - make preprocessed versions of the source
#
$(TARGET_OUTPUT_DIR)/%.i: %.cpp FORCE
	$(CXX) $< -E -o $@ $(cxx-flags)

$(TARGET_OUTPUT_DIR)/%.s: %.cpp FORCE
	$(CXX) $< -S -o $@ $(cxx-flags)

$(TARGET_OUTPUT_DIR)/%.i: %.c FORCE
	$(CC) $< -E -o $@ $(cc-flags)

####### build rules
#
#
# Provide our own rules for building...
#

WRAPPED_CC = $(CCACHE) $(CC)
WRAPPED_CXX = $(CCACHE) $(CXX)

$(TARGET_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.c $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  CC      $@"
	$(Q)$(WRAPPED_CC) $< -c -o $@ $(cc-flags)

$(TARGET_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.cpp $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(WRAPPED_CXX) $< -c -o $@ $(cxx-flags)
