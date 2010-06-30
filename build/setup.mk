# Rules for XCSoarSetup.dll

XCSOARSETUP_SOURCES = \
	$(SRC)/XCSoarSetup.cpp
XCSOARSETUP_OBJS = $(call SRC_TO_OBJ,$(XCSOARSETUP_SOURCES))

$(TARGET_OUTPUT_DIR)/XCSoarSetup.e: $(SRC)/XcSoarSetup.def $(XCSOARSETUP_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	$(Q)$(DLLTOOL) -e $@ -d $^

$(TARGET_BIN_DIR)/XCSoarSetup.dll: TARGET_LDLIBS =
$(TARGET_BIN_DIR)/XCSoarSetup.dll: $(TARGET_OUTPUT_DIR)/XCSoarSetup.e $(XCSOARSETUP_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  DLL     $@"
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
# JMW not tested yet, probably need to use dlltool?
