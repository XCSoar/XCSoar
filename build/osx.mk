ifeq ($(TARGET_IS_OSX),y)

TARGET_LDLIBS += -framework AppKit -framework CoreServices

# WORKAROUND: Apple's "AssertMacros.h" header (at least in SDK 10.12) declares a
# check() method, which conflicts with our code (and boost).
# Defining __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0 avoids this
# problem.
TARGET_CPPFLAGS += -D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0

DMG_TMPDIR = $(TARGET_OUTPUT_DIR)/dmg

MKISOFS ?= mkisofs

ifeq ($(TESTING),y)
DMG_NAME = XCSoar-testing.dmg
OSX_APP_LABEL = XCSoar Testing
OSX_LOGO = $(DATA)/graphics/logo_red_1024.icns
OSX_APP_BUNDLE_INENTIFIER = XCSoar-Testing.app
else
DMG_NAME = XCSoar.dmg
OSX_APP_LABEL = XCSoar
OSX_LOGO = $(DATA)/graphics/logo_1024.icns
OSX_APP_BUNDLE_INENTIFIER = XCSoar.app
endif

$(TARGET_OUTPUT_DIR)/$(DMG_NAME): $(TARGET_BIN_DIR)/xcsoar Data/OSX/Info.plist.in.xml $(OSX_LOGO)
	@$(NQ)echo "  DMG     $@"
	$(Q)rm -rf $(DMG_TMPDIR)
	$(Q)$(MKDIR) -p $(DMG_TMPDIR)/$(OSX_APP_BUNDLE_INENTIFIER)/Contents/MacOS
	$(Q)sed -e "s,VERSION_PLACEHOLDER,$(FULL_VERSION)," -e 's/OSX_APP_LABEL_PLACEHOLDER/$(OSX_APP_LABEL)/g' < Data/OSX/Info.plist.in.xml > $(DMG_TMPDIR)/$(OSX_APP_BUNDLE_INENTIFIER)/Contents/Info.plist
	$(Q)cp $(TARGET_BIN_DIR)/xcsoar $(DMG_TMPDIR)/$(OSX_APP_BUNDLE_INENTIFIER)/Contents/MacOS/
	$(Q)$(MKDIR) -p $(DMG_TMPDIR)/$(OSX_APP_BUNDLE_INENTIFIER)/Contents/Resources
	$(Q)cp $(OSX_LOGO) $(DMG_TMPDIR)/$(OSX_APP_BUNDLE_INENTIFIER)/Contents/Resources/logo_1024.icns
	$(Q)rm -f $@
	$(Q)$(MKISOFS) -V "$(OSX_APP_LABEL)" -quiet -D -no-pad -r -apple -o $@ $(DMG_TMPDIR)

dmg: $(TARGET_OUTPUT_DIR)/$(DMG_NAME)

endif

