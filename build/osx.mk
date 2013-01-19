ifeq ($(TARGET_IS_DARWIN),y)

HDIUTIL = hdiutil
HDIUTIL_OPTIONS =

ifneq ($(V),2)
HDIUTIL_OPTIONS += -quiet
endif

DMG_TMPDIR = $(TARGET_OUTPUT_DIR)/dmg
DMG_TMP = $(TARGET_OUTPUT_DIR)/tmp.dmg

$(DMG_TMP): $(TARGET_BIN_DIR)/xcsoar build/Info.plist $(DATA)/graphics/logo_128.icns
	@$(NQ)echo "  DMG     $@"
	$(Q)rm -rf $(DMG_TMPDIR)
	$(Q)mkdir -p $(DMG_TMPDIR)/XCSoar.app/Contents/MacOS
	$(Q)sed -e "s,VERSION,$(FULL_VERSION)," <build/Info.plist >$(DMG_TMPDIR)/XCSoar.app/Contents/Info.plist
	$(Q)cp $(TARGET_BIN_DIR)/xcsoar $(DMG_TMPDIR)/XCSoar.app/Contents/MacOS/
	$(Q)mkdir -p $(DMG_TMPDIR)/XCSoar.app/Contents/Resources
	$(Q)cp $(DATA)/graphics/logo_128.icns $(DMG_TMPDIR)/XCSoar.app/Contents/Resources/
	$(Q)rm -f $@
	$(Q)$(HDIUTIL) create $(HDIUTIL_OPTIONS) -fs HFS+ -volname "XCSoar" -srcfolder $(DMG_TMPDIR) $@

$(TARGET_OUTPUT_DIR)/XCSoar.dmg: $(DMG_TMP)
	@$(NQ)echo "  DMG     $@"
	$(Q)rm -f $@
	$(Q)$(HDIUTIL) convert $(HDIUTIL_OPTIONS) $< -format UDCO -o $@
	$(Q)$(HDIUTIL) internet-enable $(HDIUTIL_OPTIONS) -yes $@

dmg: $(TARGET_OUTPUT_DIR)/XCSoar.dmg

endif
