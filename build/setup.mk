# Rules for XCSoarSetup.dll

ifeq ($(HAVE_CE)$(findstring $(TARGET),ALTAIR),y)

XCSOARSETUP_DLL = $(TARGET_BIN_DIR)/XCSoarSetup.dll
XCSOARSETUP_SOURCES = \
	$(SRC)/XCSoarSetup.cpp
XCSOARSETUP_OBJS = $(call SRC_TO_OBJ,$(XCSOARSETUP_SOURCES))

$(TARGET_OUTPUT_DIR)/XCSoarSetup.e: $(SRC)/XcSoarSetup.def $(XCSOARSETUP_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	$(Q)$(DLLTOOL) -e $@ -d $^

$(XCSOARSETUP_DLL): TARGET_LDLIBS =
$(XCSOARSETUP_DLL): $(TARGET_OUTPUT_DIR)/XCSoarSetup.e $(XCSOARSETUP_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  DLL     $@"
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
# JMW not tested yet, probably need to use dlltool?

else

XCSOARSETUP_DLL =

endif
