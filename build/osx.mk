ifeq ($(TARGET_IS_OSX),y)

TARGET_LDLIBS += -framework AppKit

DMG_TMPDIR = $(TARGET_OUTPUT_DIR)/dmg

MKISOFS ?= mkisofs

$(TARGET_OUTPUT_DIR)/XCSoar.dmg: $(TARGET_BIN_DIR)/xcsoar Data/OSX/Info.plist $(DATA)/graphics/logo_128.icns
	@$(NQ)echo "  DMG     $@"
	$(Q)rm -rf $(DMG_TMPDIR)
	$(Q)$(MKDIR) -p $(DMG_TMPDIR)/XCSoar.app/Contents/MacOS
	$(Q)sed -e "s,VERSION,$(FULL_VERSION)," < Data/OSX/Info.plist >$(DMG_TMPDIR)/XCSoar.app/Contents/Info.plist
	$(Q)cp $(TARGET_BIN_DIR)/xcsoar $(DMG_TMPDIR)/XCSoar.app/Contents/MacOS/
	$(Q)$(MKDIR) -p $(DMG_TMPDIR)/XCSoar.app/Contents/Resources
	$(Q)cp $(DATA)/graphics/logo_128.icns $(DMG_TMPDIR)/XCSoar.app/Contents/Resources/
	$(Q)rm -f $@
	$(Q)$(MKISOFS) -V "XCSoar" -quiet -D -no-pad -r -apple -o $@ $(DMG_TMPDIR)

dmg: $(TARGET_OUTPUT_DIR)/XCSoar.dmg

endif

