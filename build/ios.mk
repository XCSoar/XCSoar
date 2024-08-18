ifeq ($(TARGET_IS_IOS),y)

TARGET_LDLIBS += -framework UIKit

IPA_TMPDIR = $(TARGET_OUTPUT_DIR)/ipa

ifeq ($(TESTING),y)
IPA_NAME = xcsoar-testing.ipa
IOS_APP_DIR_NAME = XCSoar.testing.app
IOS_APP_BUNDLE_IDENTIFIER = XCSoar-testing
IOS_APP_DISPLAY_NAME = XCSoar Testing
IOS_ICON_SVG = $(topdir)/Data/iOS/iOS-Icon_red.svg
IOS_SPLASH_BASE_IMG=$(DATA)/graphics/logo_red_320.png
IOS_GRAPHICS_DIR=$(DATA)/ios-graphics-testing
else
IPA_NAME = xcsoar.ipa
IOS_APP_DIR_NAME = XCSoar.app
IOS_APP_BUNDLE_IDENTIFIER = XCSoar
IOS_APP_DISPLAY_NAME = XCSoar
IOS_ICON_SVG = $(topdir)/Data/iOS/iOS-Icon.svg
IOS_SPLASH_BASE_IMG=$(DATA)/graphics/logo_320.png
IOS_GRAPHICS_DIR=$(DATA)/ios-graphics
endif

ifeq ($(findstring aarch64,$(HOST_TRIPLET)),aarch64)
IOS_INFO_PLIST_ARCH_PLACEHOLDER = arm64
else ifeq ($(findstring armv7,$(HOST_TRIPLET)),armv7)
IOS_INFO_PLIST_ARCH_PLACEHOLDER = armv7
else
$(error Could not determine correct architecture identifier for Info.plist)
endif

IOS_GRAPHICS = \
	$(IOS_GRAPHICS_DIR)/Default.png \
	$(IOS_GRAPHICS_DIR)/Default@2x.png \
	$(IOS_GRAPHICS_DIR)/Default-568h@2x.png \
	$(IOS_GRAPHICS_DIR)/Default-667h@2x.png \
	$(IOS_GRAPHICS_DIR)/Default-736h@3x.png \
	$(IOS_GRAPHICS_DIR)/Default-Portrait.png \
	$(IOS_GRAPHICS_DIR)/Default-Landscape.png \
	$(IOS_GRAPHICS_DIR)/Default-Portrait@2x.png \
	$(IOS_GRAPHICS_DIR)/Default-Landscape@2x.png \
	$(IOS_GRAPHICS_DIR)/Default-Landscape-667h@2x.png \
	$(IOS_GRAPHICS_DIR)/Default-Landscape-736h@3x.png \
	$(IOS_GRAPHICS_DIR)/Icon.png \
	$(IOS_GRAPHICS_DIR)/Icon-72.png \
	$(IOS_GRAPHICS_DIR)/Icon@2x.png

$(IOS_GRAPHICS_DIR)/Default.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 320x480 $@

$(IOS_GRAPHICS_DIR)/Default@2x.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 640x960 $@

$(IOS_GRAPHICS_DIR)/Default-568h@2x.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 640x1136 $@

$(IOS_GRAPHICS_DIR)/Default-667h@2x.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 750x1334 $@

$(IOS_GRAPHICS_DIR)/Default-736h@3x.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 1242x2208 $@

$(IOS_GRAPHICS_DIR)/Default-Portrait.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 768x1004 $@

$(IOS_GRAPHICS_DIR)/Default-Landscape.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 1024x748 $@

$(IOS_GRAPHICS_DIR)/Default-Portrait@2x.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 1536x2008 $@

$(IOS_GRAPHICS_DIR)/Default-Landscape@2x.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 2048x1496 $@

$(IOS_GRAPHICS_DIR)/Default-Landscape-667h@2x.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 1334x750 $@

$(IOS_GRAPHICS_DIR)/Default-Landscape-736h@3x.png: $(IOS_SPLASH_BASE_IMG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)$(IM_PREFIX)convert $(IOS_SPLASH_BASE_IMG) -background white -gravity center -extent 2208x1242 $@

$(IOS_GRAPHICS_DIR)/Icon.png: $(IOS_ICON_SVG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)rsvg-convert $< -w 57 -h 57 -a -o $@

$(IOS_GRAPHICS_DIR)/Icon-72.png: $(IOS_ICON_SVG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)rsvg-convert $< -w 72 -h 72 -a -o $@

$(IOS_GRAPHICS_DIR)/Icon@2x.png: $(IOS_ICON_SVG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)rsvg-convert $< -w 114 -h 114 -a -o $@


$(TARGET_OUTPUT_DIR)/Info.plist.xml: $(topdir)/Data/iOS/Info.plist.in.xml | $(TARGET_OUTPUT_DIR)/dirstamp
	$(Q)sed -e 's/IOS_APP_DISPLAY_NAME_PLACEHOLDER/$(IOS_APP_DISPLAY_NAME)/g' \
		-e 's/IOS_APP_BUNDLE_IDENTIFIER_PLACEHOLDER/$(IOS_APP_BUNDLE_IDENTIFIER)/g' \
		-e 's/VERSION_PLACEHOLDER/$(IOS_APP_VERSION)/g' \
		-e 's/IOS_ARCH_PLACEHOLDER/$(IOS_INFO_PLIST_ARCH_PLACEHOLDER)/g' \
		$< > $@

$(TARGET_OUTPUT_DIR)/Info.plist: $(TARGET_OUTPUT_DIR)/Info.plist.xml
ifeq ($(HOST_IS_DARWIN),y)
	$(Q)plutil -convert binary1 -o $@ $<
else
	$(Q)plistutil -i $< -o $@
endif


$(TARGET_OUTPUT_DIR)/$(IPA_NAME): $(TARGET_BIN_DIR)/xcsoar $(TARGET_OUTPUT_DIR)/Info.plist  $(IOS_GRAPHICS)
	@$(NQ)echo "  IPA     $@"
	$(Q)rm -rf $(IPA_TMPDIR)
	$(Q)$(MKDIR) -p $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)
	$(Q)cp $(TARGET_BIN_DIR)/xcsoar $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/XCSoar
	$(Q)cp $(TARGET_OUTPUT_DIR)/Info.plist $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)
	$(Q)cp $(IOS_GRAPHICS) $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)
	$(Q)cd $(IPA_TMPDIR) && $(ZIP) -r ../$(IPA_NAME) ./*

ipa: $(TARGET_OUTPUT_DIR)/$(IPA_NAME)

endif
