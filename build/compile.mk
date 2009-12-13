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
DEPFLAGS = -Wp,-MD,$(DEPFILE)
dirtarget = $(subst \\,_,$(subst /,_,$(dir $@)))
cc-flags = $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH)
cxx-flags = $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH)

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
	$(Q)$(CC) $(cc-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET).o: %.cpp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET).o: %.cxx
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET)-Simulator.o: %.c
	@$(NQ)echo "  CC      $@"
	$(Q)$(CC) $(cc-flags) -D_SIM_ -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET)-Simulator.o: %.cpp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -D_SIM_ -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET)-Simulator.o: %.cxx
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -D_SIM_ -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)
