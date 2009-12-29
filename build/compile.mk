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

####### dependency handling

DEPFILE = $(dir $@).$(notdir $@).d
DEPFLAGS = -Wp,-MD,$(DEPFILE),-MT,$@
dirtarget = $(subst \\,_,$(subst /,_,$(dir $@)))
cc-flags = $(DEPFLAGS) $(ALL_CFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) $(FLAGS_COVERAGE)
cxx-flags = $(DEPFLAGS) $(ALL_CXXFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) $(FLAGS_COVERAGE)

#
# Useful debugging targets - make preprocessed versions of the source
#
%.i: %.cpp FORCE
	$(CXX) $(cxx-flags) -E $(OUTPUT_OPTION) $<

%.s: %.cpp FORCE
	$(CXX) $(cxx-flags) -S $(OUTPUT_OPTION) $<

%.i: %.c FORCE
	$(CC) $(cc-flags) -E $(OUTPUT_OPTION) $<

####### build rules
#
#
# Provide our own rules for building...
#
%-$(TARGET).o: %.c
	@$(NQ)echo "  CC      $@"
	$(Q)$(CC) -c -o $@ $(cc-flags) $<

%-$(TARGET).o: %.cpp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) -c -o $@ $(cxx-flags) $<

%-$(TARGET)-Simulator.o: %.c
	@$(NQ)echo "  CC      $@"
	$(Q)$(CC) -c -o $@ $(cc-flags) -D_SIM_ $<

%-$(TARGET)-Simulator.o: %.cpp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) -c -o $@ $(cxx-flags) -D_SIM_ $<
