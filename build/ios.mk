ifeq ($(TARGET_IS_IOS),y)

TARGET_LDLIBS += -framework UIKit

IPA_TMPDIR = $(TARGET_OUTPUT_DIR)/ipa

ifeq ($(TESTING),y)
IPA_NAME = xcsoar-testing.ipa
IOS_LOGO_SVG = $(topdir)/Data/graphics/logo_red.svg
IOS_ICON_SVG = $(topdir)/Data/iOS/iOS-Icon_red.svg
IOS_APP_DIR_NAME = XCSoar.testing.app
IOS_APP_BUNDLE_INENTIFIER = XCSoar-testing
IOS_APP_DISPLAY_NAME = XCSoar Testing
else
IPA_NAME = xcsoar.ipa
IOS_LOGO_SVG = $(topdir)/Data/graphics/logo.svg
IOS_ICON_SVG = $(topdir)/Data/iOS/iOS-Icon.svg
IOS_APP_DIR_NAME = XCSoar.app
IOS_APP_BUNDLE_INENTIFIER = XCSoar
IOS_APP_DISPLAY_NAME = XCSoar
endif

$(TARGET_OUTPUT_DIR)/$(IPA_NAME): $(TARGET_BIN_DIR)/xcsoar $(IOS_LOGO_SVG) $(topdir)/Data/iOS/Info.plist.in.xml
	@$(NQ)echo "  IPA     $@"
	$(Q)rm -rf $(IPA_TMPDIR)
	$(Q)$(MKDIR) -p $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 320 -h 320 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default.png \
		-resize 320x480 -background white -gravity center -extent 320x480 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 640 -h 640 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default@2x.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default@2x.png \
		-resize 640x960 -background white -gravity center -extent 640x960 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default@2x.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 640 -h 640 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-568h@2x.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-568h@2x.png \
		-resize 640x1136 -background white -gravity center -extent 640x1136 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-568h@2x.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 750 -h 750 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-667h@2x.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-667h@2x.png \
		-resize 750x1334 -background white -gravity center -extent 750x1334 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-667h@2x.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 1242 -h 1242 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-736h@3x.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-736h@3x.png \
		-resize 1242x2208 -background white -gravity center -extent 1242x2208 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-736h@3x.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 768 -h 768 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Portrait.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Portrait.png \
		-resize 768x1004 -background white -gravity center -extent 768x1004 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Portrait.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 748 -h 748 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape.png \
		-resize 1024x748 -background white -gravity center -extent 1024x748 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 1536 -h 1536 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Portrait@2x.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Portrait@2x.png \
		-resize 1536x2008 -background white -gravity center -extent 1536x2008 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Portrait@2x.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 1496 -h 1496 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape@2x.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape@2x.png \
		-resize 2048x1496 -background white -gravity center -extent 2048x1496 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape@2x.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 750 -h 750 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape-667h@2x.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape-667h@2x.png \
		-resize 1334x750 -background white -gravity center -extent 1334x750 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape-667h@2x.png
	$(Q)rsvg-convert $(IOS_LOGO_SVG) -w 1242 -h 1242 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape-736h@3x.png
	$(Q)$(IM_PREFIX)convert $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape-736h@3x.png \
		-resize 2208x1242 -background white -gravity center -extent 2208x1242 \
		$(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Default-Landscape-736h@3x.png
	$(Q)rsvg-convert $(IOS_ICON_SVG) -w 57 -h 57 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Icon.png
	$(Q)rsvg-convert $(IOS_ICON_SVG) -w 72 -h 72 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Icon-72.png
	$(Q)rsvg-convert $(IOS_ICON_SVG) -w 114 -h 114 -a \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Icon@2x.png
	$(Q)sed -e 's/IOS_APP_DISPLAY_NAME_PLACEHOLDER/$(IOS_APP_DISPLAY_NAME)/g' \
		-e 's/IOS_APP_BUNDLE_INENTIFIER_PLACEHOLDER/$(IOS_APP_BUNDLE_INENTIFIER)/g' \
		-e 's/VERSION_PLACEHOLDER/$(IOS_APP_VERSION)/g' \
		$(topdir)/Data/iOS/Info.plist.in.xml \
		> $(TARGET_OUTPUT_DIR)/Info.plist.xml
ifeq ($(HOST_IS_DARWIN),y)
	$(Q)plutil -convert binary1 \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Info.plist \
		$(TARGET_OUTPUT_DIR)/Info.plist.xml
else
	$(Q)plistutil -i $(TARGET_OUTPUT_DIR)/Info.plist.xml \
		-o $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/Info.plist
endif
	$(Q)cp $(TARGET_BIN_DIR)/xcsoar $(IPA_TMPDIR)/Payload/$(IOS_APP_DIR_NAME)/XCSoar
	$(Q)cd $(IPA_TMPDIR) && zip -r -q ../$(IPA_NAME) ./*

ipa: $(TARGET_OUTPUT_DIR)/$(IPA_NAME)

endif

