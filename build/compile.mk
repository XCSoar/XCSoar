######## tools

EXE := $(findstring .exe,$(MAKE))
AR = $(TCPATH)ar$(EXE)
CXX = $(TCPATH)g++$(EXE)
CC = $(TCPATH)gcc$(EXE)
SIZE = $(TCPATH)size$(EXE)
STRIP = $(TCPATH)strip$(EXE)
WINDRES = $(TCPATH)windres$(EXE)
ARFLAGS = r

ifeq ($(CONFIG_WINE),y)
AR = ar$(EXE)
STRIP = strip$(EXE)
WINDRES = wrc$(EXE)
endif

####### paths

OBJ_SUFFIX = .o

# Converts a list of source file names to *.o
SRC_TO_OBJ = $(patsubst %.cpp,%$(OBJ_SUFFIX),$(patsubst %.c,%$(OBJ_SUFFIX),$(addprefix $(TARGET_OUTPUT_DIR)/,$(1))))

####### dependency handling

DEPFILE = $(@:.o=.d)
DEPFLAGS = -Wp,-MD,$(DEPFILE),-MT,$@
dirtarget = $(subst \\,_,$(subst /,_,$(dir $@)))
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

$(TARGET_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.c $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  CC      $@"
	$(Q)$(CC) -c -o $@ $(cc-flags) $<

$(TARGET_OUTPUT_DIR)/%$(OBJ_SUFFIX): %.cpp $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) -c -o $@ $(cxx-flags) $<

$(TARGET_OUTPUT_DIR)/%-Simulator$(OBJ_SUFFIX): %.c $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  CC      $@"
	$(Q)$(CC) -c -o $@ $(cc-flags) -D_SIM_ $<

$(TARGET_OUTPUT_DIR)/%-Simulator$(OBJ_SUFFIX): %.cpp $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) -c -o $@ $(cxx-flags) -D_SIM_ $<
