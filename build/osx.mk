ifeq ($(shell uname -s),Darwin)

HDIUTIL = hdiutil

DMG_TMPDIR = $(TARGET_OUTPUT_DIR)/dmg
DMG_TMP = $(TARGET_OUTPUT_DIR)/tmp.dmg

$(DMG_TMP): $(TARGET_BIN_DIR)/xcsoar build/Info.plist output/data/graphics/xcsoarswiftsplash_128.icns
	rm -rf $(DMG_TMPDIR)
	mkdir -p $(DMG_TMPDIR)/XCSoar.app/Contents/MacOS
	sed -e "s,VERSION,$(FULL_VERSION)," <build/Info.plist >$(DMG_TMPDIR)/XCSoar.app/Contents/Info.plist
	cp $(TARGET_BIN_DIR)/xcsoar $(DMG_TMPDIR)/XCSoar.app/Contents/MacOS/
	mkdir -p $(DMG_TMPDIR)/XCSoar.app/Contents/Resources
	cp output/data/graphics/xcsoarswiftsplash_128.icns $(DMG_TMPDIR)/XCSoar.app/Contents/Resources/
	$(HDIUTIL) create -fs HFS+ -volname "XCSoar" -srcfolder $(DMG_TMPDIR) $@

$(TARGET_OUTPUT_DIR)/XCSoar.dmg: $(DMG_TMP)
	$(HDIUTIL) convert $< -format UDCO -o $@
	$(HDIUTIL) internet-enable -yes $@

endif
