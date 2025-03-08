ifeq ($(TARGET_IS_IOS),y)

TARGET_LDLIBS += -framework UIKit

IPA_TMPDIR = $(TARGET_OUTPUT_DIR)/ipa

ifeq ($(TESTING),y)
IPA_NAME = xcsoar-testing.ipa
IOS_APP_DIR_NAME = XCSoar.testing.app
IOS_APP_BUNDLE_IDENTIFIER ?= XCSoar-testing
IOS_APP_DISPLAY_NAME = XCSoar Testing
IOS_ICON_SVG = $(topdir)/Data/iOS/iOS-Icon_red.svg
IOS_SPLASH_BASE_IMG=$(DATA)/graphics/logo_red_320.png
IOS_GRAPHICS_DIR=$(DATA)/ios-graphics-testing
else
IPA_NAME = xcsoar.ipa
IOS_APP_DIR_NAME = XCSoar.app
IOS_APP_BUNDLE_IDENTIFIER ?= XCSoar
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
	$(IOS_GRAPHICS_DIR)/Assets.car \
	$(IOS_GRAPHICS_DIR)/LaunchScreen.storyboardc

$(IOS_GRAPHICS_DIR)/Assets.car: $(topdir)/Data/iOS/Assets.xcassets
# will also generate $(IOS_GRAPHICS_DIR)/AppIcon%.png, but can't combine implicit and explicit rules
	mkdir -p $(IOS_GRAPHICS_DIR)
	xcrun actool $< --compile $(dir $@) --platform iphoneos --minimum-deployment-target 8.0 --app-icon AppIcon --output-partial-info-plist $(IOS_GRAPHICS_DIR)/assets-partial.plist

$(IOS_GRAPHICS_DIR)/LaunchScreen.storyboardc: $(topdir)/Data/iOS/LaunchScreen.storyboard
	mkdir -p $(IOS_GRAPHICS_DIR)
	ibtool $< --compile $@

HOST_MACOS_VERSION = $(shell sw_vers -buildVersion)
TARGET_SDK_NAME = $(shell /usr/libexec/PlistBuddy -c 'print CanonicalName' $(DARWIN_SDK)/SDKSettings.plist)
TARGET_SDK_VERSION = $(shell /usr/libexec/PlistBuddy -c 'print Version' $(DARWIN_SDK)/SDKSettings.plist)
XCODE_VERSION = $(shell xcodebuild -version | grep Xcode | cut -d ' ' -f 2)
XCODE_VERSION_FORMATTED = $(shell printf "%02.2f" $(XCODE_VERSION) | tr -d '.')
SDK_BUILD_VERSION = $(shell /usr/libexec/PlistBuddy -c 'print ProductBuildVersion' $(DARWIN_SDK)/System/Library/CoreServices/SystemVersion.plist)
XCODE_BUILD = $(shell /usr/libexec/PlistBuddy -c 'print ProductBuildVersion' $(shell xcode-select --print-path)/../version.plist)
OSX_MIN_SUPPORTED_VERSION ?= 11.0


$(TARGET_OUTPUT_DIR)/Info.plist.xml: $(topdir)/Data/iOS/Info.plist.in.xml | $(TARGET_OUTPUT_DIR)/dirstamp
	$(Q)sed -e 's/IOS_APP_DISPLAY_NAME_PLACEHOLDER/$(IOS_APP_DISPLAY_NAME)/g' \
		-e 's/IOS_APP_BUNDLE_IDENTIFIER_PLACEHOLDER/$(IOS_APP_BUNDLE_IDENTIFIER)/g' \
		-e 's/IOS_MIN_SUPPORTED_VERSION_PLACEHOLDER/$(IOS_MIN_SUPPORTED_VERSION)/g' \
		-e 's/MACOS_MIN_SUPPORTED_VERSION_PLACEHOLDER/$(OSX_MIN_SUPPORTED_VERSION)/g' \
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
	$(Q)cp -r $(IOS_GRAPHICS_DIR)/. $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)
	$(Q)cd $(IPA_TMPDIR) && $(ZIP) -r ../$(IPA_NAME) ./*

ipa: $(TARGET_OUTPUT_DIR)/$(IPA_NAME)

endif
