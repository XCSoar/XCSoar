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
	$(IOS_GRAPHICS_DIR)/Icon-1024.png \
	$(IOS_GRAPHICS_DIR)/Icon@2x.png \
	$(IOS_GRAPHICS_DIR)/Assets.car \
	$(IOS_GRAPHICS_DIR)/AppIcon60x60@2x.png \
	$(IOS_GRAPHICS_DIR)/AppIcon76x76@2x~ipad.png

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

$(IOS_GRAPHICS_DIR)/Icon-76.png: $(IOS_ICON_SVG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)rsvg-convert $< -w 76 -h 76 -a -o $@

$(IOS_GRAPHICS_DIR)/Icon@2x.png: $(IOS_ICON_SVG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)rsvg-convert $< -w 114 -h 114 -a -o $@

$(IOS_GRAPHICS_DIR)/Icon-120.png: $(IOS_ICON_SVG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)rsvg-convert $< -w 120 -h 120 -a -o $@

$(IOS_GRAPHICS_DIR)/Icon-152.png: $(IOS_ICON_SVG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)rsvg-convert $< -w 152 -h 152 -a -o $@

$(IOS_GRAPHICS_DIR)/Icon-1024.png: $(IOS_ICON_SVG) | $(IOS_GRAPHICS_DIR)/dirstamp
	$(Q)rsvg-convert $< -w 1024 -h 1024 -a -b white -o $@

$(IOS_GRAPHICS_DIR)/Assets.car: $(topdir)/Data/iOS/Assets.xcassets
# will also generate $(IOS_GRAPHICS_DIR)/AppIcon%.png, but can't combine implicit and explicit rules
	xcrun actool $< --compile $(dir $@) --platform iphoneos --minimum-deployment-target 8.0 --app-icon AppIcon --output-partial-info-plist $(IOS_GRAPHICS_DIR)/assets-partial.plist

HOST_MACOS_VERSION = $(shell sw_vers -buildVersion)
TARGET_SDK_NAME = $(shell /usr/libexec/PlistBuddy -c 'print CanonicalName' $(DARWIN_SDK)/SDKSettings.plist)
TARGET_SDK_VERSION = $(shell /usr/libexec/PlistBuddy -c 'print Version' $(DARWIN_SDK)/SDKSettings.plist)
XCODE_VERSION = $(shell xcodebuild -version | grep Xcode | cut -d ' ' -f 2)
XCODE_VERSION_FORMATTED = $(shell printf "%02.2f" $(XCODE_VERSION) | tr -d '.')
SDK_BUILD_VERSION = $(shell /usr/libexec/PlistBuddy -c 'print ProductBuildVersion' $(DARWIN_SDK)/System/Library/CoreServices/SystemVersion.plist)
XCODE_BUILD = $(shell /usr/libexec/PlistBuddy -c 'print ProductBuildVersion' $(shell xcode-select --print-path)/../version.plist)


$(TARGET_OUTPUT_DIR)/Info.plist.xml: $(topdir)/Data/iOS/Info.plist.in.xml | $(TARGET_OUTPUT_DIR)/dirstamp
	$(Q)sed -e 's/IOS_APP_DISPLAY_NAME_PLACEHOLDER/$(IOS_APP_DISPLAY_NAME)/g' \
		-e 's/IOS_APP_BUNDLE_IDENTIFIER_PLACEHOLDER/$(IOS_APP_BUNDLE_IDENTIFIER)/g' \
		-e 's/IOS_MIN_SUPPORTED_VERSION_PLACEHOLDER/$(IOS_MIN_SUPPORTED_VERSION)/g' \
		-e 's/XCSOAR_VERSION_PLACEHOLDER/$(IOS_APP_VERSION)/g' \
		-e 's/BUILD_NUMBER_PLACEHOLDER/$(IOS_APP_BUILD_NUMBER)/g' \
		-e 's/IOS_ARCH_PLACEHOLDER/$(IOS_INFO_PLIST_ARCH_PLACEHOLDER)/g' \
		-e 's/BUILD_MACHINE_OS_BUILD_PLACEHOLDER/$(HOST_MACOS_VERSION)/g' \
		-e 's/SDK_NAME_PLACEHOLDER/$(TARGET_SDK_NAME)/g' \
		-e 's/XCODE_VERSION_PLACEHOLDER/$(XCODE_VERSION_FORMATTED)/g' \
		-e 's/SDK_BUILD_PLACEHOLDER/$(SDK_BUILD_VERSION)/g' \
		-e 's/PLATFORM_VERSION_PLACEHOLDER/$(TARGET_SDK_VERSION)/g' \
		-e 's/XCODE_BUILD_PLACEHOLDER/$(XCODE_BUILD)/g' \
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
