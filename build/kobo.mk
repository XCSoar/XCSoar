$(eval $(call pkg-config-library,LIBCRYPTO,libcrypto))

KOBO_MENU_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Asset.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Hardware/CPU.cpp \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(SRC)/Hardware/RotateDisplay.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ui/control/TerminalWindow.cpp \
	$(SRC)/Look/TerminalLook.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Renderer/TwoTextRowsRenderer.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Dialogs/HelpDialog.cpp \
	$(SRC)/Dialogs/Message.cpp \
	$(SRC)/Dialogs/Error.cpp \
	$(SRC)/Dialogs/LockScreen.cpp \
	$(SRC)/Dialogs/TextEntry.cpp \
	$(SRC)/Dialogs/KnobTextEntry.cpp \
	$(SRC)/Dialogs/TouchTextEntry.cpp \
	$(SRC)/Dialogs/SimulatorPromptWindow.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(SRC)/Kobo/WPASupplicant.cpp \
	$(SRC)/Kobo/System.cpp \
	$(SRC)/Kobo/Kernel.cpp \
	$(SRC)/Kobo/NetworkDialog.cpp \
	$(SRC)/Kobo/SystemDialog.cpp \
	$(SRC)/Kobo/ToolsDialog.cpp \
	$(SRC)/Kobo/WPASupplicant.cpp \
	$(SRC)/Kobo/WifiDialog.cpp \
	$(SRC)/Kobo/FakeSymbols.cpp \
	$(SRC)/Kobo/KoboMenu.cpp
KOBO_MENU_DEPENDS = WIDGET FORM EVENT RESOURCE ASYNC LIBNET OS IO THREAD MATH UTIL LIBCRYPTO
KOBO_MENU_STRIP = y

$(eval $(call link-program,KoboMenu,KOBO_MENU))

ifeq ($(TARGET),UNIX)
OPTIONAL_OUTPUTS += $(KOBO_MENU_BIN)
endif

ifeq ($(TARGET_IS_KOBO),y)

KOBO_POWER_OFF_SOURCES = \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(SRC)/Hardware/RotateDisplay.cpp \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(SRC)/Hardware/Battery.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Logger/FlightParser.cpp \
	$(SRC)/Renderer/FlightListRenderer.cpp \
	$(SRC)/Renderer/TextRenderer.cpp \
	$(SRC)/FlightInfo.cpp \
	$(SRC)/Kobo/PowerOff.cpp
KOBO_POWER_OFF_LDADD = $(FAKE_LIBS)
KOBO_POWER_OFF_DEPENDS = SCREEN EVENT RESOURCE IO ASYNC OS MATH UTIL TIME
KOBO_POWER_OFF_STRIP = y
$(eval $(call link-program,PowerOff,KOBO_POWER_OFF))
OPTIONAL_OUTPUTS += $(KOBO_POWER_OFF_BIN)

BITSTREAM_VERA_DIR ?= $(shell \
if [ -d /usr/share/fonts/truetype/ttf-bitstream-vera ]; then \
	echo /usr/share/fonts/truetype/ttf-bitstream-vera; \
elif [ -d /usr/share/fonts/bitstream-vera ]; then \
	echo /usr/share/fonts/bitstream-vera; \
fi)
BITSTREAM_VERA_NAMES = Vera VeraBd VeraIt VeraBI VeraMono
BITSTREAM_VERA_FILES = $(patsubst %,$(BITSTREAM_VERA_DIR)/%.ttf,$(BITSTREAM_VERA_NAMES))

THIRDPARTY_TOOL_NAMES = simple_usbmodeswitch
THIRDPARTY_TOOL_FILES = $(addprefix $(THIRDPARTY_LIBS_ROOT)/bin/,$(THIRDPARTY_TOOL_NAMES))

KOBO_KERNEL_URL = https://download.xcsoar.org/contrib/kobo/kernel/kobo-1.0.uImage
KOBO_KERNEL_ALTERNATIVE_URL = $(KOBO_KERNEL_URL)
KOBO_KERNEL_MD5 = 207f1b732330ac0bdc9c1fa1d16dd402
KOBO_KERNEL_DOWNLOAD = $(DOWNLOAD_DIR)/$(notdir $(KOBO_KERNEL_URL))

$(KOBO_KERNEL_DOWNLOAD): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(KOBO_KERNEL_URL) $(KOBO_KERNEL_ALTERNATIVE_URL) $(KOBO_KERNEL_MD5) $(DOWNLOAD_DIR)

KOBOOTG_KERNEL_URL = https://download.xcsoar.org/contrib/kobo/kernel/kobootg-1.0.uImage
KOBOOTG_KERNEL_ALTERNATIVE_URL = $(KOBOOTG_KERNEL_URL)
KOBOOTG_KERNEL_MD5 = 7415607d93edb1426a10e278abc6919c
KOBOOTG_KERNEL_DOWNLOAD = $(DOWNLOAD_DIR)/$(notdir $(KOBOOTG_KERNEL_URL))

$(KOBOOTG_KERNEL_DOWNLOAD): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(KOBOOTG_KERNEL_URL) $(KOBOOTG_KERNEL_ALTERNATIVE_URL) $(KOBOOTG_KERNEL_MD5) $(DOWNLOAD_DIR)

GLOHD_KERNEL_URL = https://download.xcsoar.org/contrib/kobo/kernel/glohd-1.0.uImage
GLOHD_KERNEL_ALTERNATIVE_URL = $(GLOHD_KERNEL_URL)
GLOHD_KERNEL_MD5 = a5492fc68304ee6c35d7069854d60454
GLOHD_KERNEL_DOWNLOAD = $(DOWNLOAD_DIR)/$(notdir $(GLOHD_KERNEL_URL))

$(GLOHD_KERNEL_DOWNLOAD): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(GLOHD_KERNEL_URL) $(GLOHD_KERNEL_ALTERNATIVE_URL) $(GLOHD_KERNEL_MD5) $(DOWNLOAD_DIR)

GLOHDOTG_KERNEL_URL = https://download.xcsoar.org/contrib/kobo/kernel/glohdotg-1.0.uImage
GLOHDOTG_KERNEL_ALTERNATIVE_URL = $(GLOHDOTG_KERNEL_URL)
GLOHDOTG_KERNEL_MD5 = 8b2b90ab79db2493e6ec8871cd22d333
GLOHDOTG_KERNEL_DOWNLOAD = $(DOWNLOAD_DIR)/$(notdir $(GLOHDOTG_KERNEL_URL))

$(GLOHDOTG_KERNEL_DOWNLOAD): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(GLOHDOTG_KERNEL_URL) $(GLOHDOTG_KERNEL_ALTERNATIVE_URL) $(GLOHDOTG_KERNEL_MD5) $(DOWNLOAD_DIR)

AURA2_KERNEL_URL = https://download.xcsoar.org/contrib/kobo/kernel/aura2-1.0.uImage
AURA2_KERNEL_ALTERNATIVE_URL = $(AURA2_KERNEL_URL)
AURA2_KERNEL_MD5 = c2d203577e27c6d3778d99bcf5143a7e
AURA2_KERNEL_DOWNLOAD = $(DOWNLOAD_DIR)/$(notdir $(AURA2_KERNEL_URL))

$(AURA2_KERNEL_DOWNLOAD): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(AURA2_KERNEL_URL) $(AURA2_KERNEL_ALTERNATIVE_URL) $(AURA2_KERNEL_MD5) $(DOWNLOAD_DIR)

AURA2OTG_KERNEL_URL = https://download.xcsoar.org/contrib/kobo/kernel/aura2otg-1.0.uImage
AURA2OTG_KERNEL_ALTERNATIVE_URL = $(AURA2OTG_KERNEL_URL)
AURA2OTG_KERNEL_MD5 = 32f060b5cb37da6a47aea70005cfe5c1
AURA2OTG_KERNEL_DOWNLOAD = $(DOWNLOAD_DIR)/$(notdir $(AURA2OTG_KERNEL_URL))

$(AURA2OTG_KERNEL_DOWNLOAD): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(AURA2OTG_KERNEL_URL) $(AURA2OTG_KERNEL_ALTERNATIVE_URL) $(AURA2OTG_KERNEL_MD5) $(DOWNLOAD_DIR)

CLARAHD_DRIVERS_URL = https://download.xcsoar.org/contrib/kobo/kernel/clarahd_drivers-1.0.tgz
CLARAHD_DRIVERS_ALTERNATIVE_URL = $(CLARAHD_DRIVERS_URL)
CLARAHD_DRIVERS_MD5 = 203e7ff4f850105a05fa10edde0b5c40
CLARAHD_DRIVERS_DOWNLOAD = $(DOWNLOAD_DIR)/$(notdir $(CLARAHD_DRIVERS_URL))

$(CLARAHD_DRIVERS_DOWNLOAD): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(CLARAHD_DRIVERS_URL) $(CLARAHD_DRIVERS_ALTERNATIVE_URL) $(CLARAHD_DRIVERS_MD5) $(DOWNLOAD_DIR)

# /mnt/onboard/.kobo/KoboRoot.tgz is a file that is picked up by
# /etc/init.d/rcS, extracted to / on each boot; we can use it to
# install XCSoar
$(TARGET_OUTPUT_DIR)/KoboRoot.tgz: $(XCSOAR_BIN) \
	$(KOBO_MENU_BIN) $(KOBO_POWER_OFF_BIN) \
	$(BITSTREAM_VERA_FILES) \
	$(topdir)/kobo/inittab $(topdir)/kobo/rcS $(topdir)/kobo/udev.rules \
	$(KOBO_KERNEL_DOWNLOAD) $(KOBOOTG_KERNEL_DOWNLOAD) \
	$(GLOHD_KERNEL_DOWNLOAD) $(GLOHDOTG_KERNEL_DOWNLOAD) \
	$(AURA2_KERNEL_DOWNLOAD) $(AURA2OTG_KERNEL_DOWNLOAD) \
	$(CLARAHD_DRIVERS_DOWNLOAD)
	@$(NQ)echo "  TAR     $@"
	$(Q)rm -rf $(@D)/KoboRoot
	$(Q)install -m 0755 -d $(@D)/KoboRoot/etc/udev/rules.d $(@D)/KoboRoot/opt/xcsoar/bin $(@D)/KoboRoot/opt/xcsoar/lib/kernel $(@D)/KoboRoot/opt/xcsoar/share/fonts
	$(Q)install -m 0755 $(XCSOAR_BIN) $(KOBO_MENU_BIN) $(KOBO_POWER_OFF_BIN) $(topdir)/kobo/rcS $(@D)/KoboRoot/opt/xcsoar/bin
	$(Q)install -m 0755 --strip --strip-program=$(STRIP) $(THIRDPARTY_TOOL_FILES) $(@D)/KoboRoot/opt/xcsoar/bin
	$(Q)if test -f $(KOBO_KERNEL_DOWNLOAD); then install -T -m 0644 $(KOBO_KERNEL_DOWNLOAD) $(@D)/KoboRoot/opt/xcsoar/lib/kernel/uImage.kobo; fi
	$(Q)if test -f $(KOBOOTG_KERNEL_DOWNLOAD); then install -T -m 0644 $(KOBOOTG_KERNEL_DOWNLOAD) $(@D)/KoboRoot/opt/xcsoar/lib/kernel/uImage.otg; fi
	$(Q)if test -f $(GLOHD_KERNEL_DOWNLOAD); then install -T -m 0644 $(GLOHD_KERNEL_DOWNLOAD) $(@D)/KoboRoot/opt/xcsoar/lib/kernel/uImage.glohd; fi
	$(Q)if test -f $(GLOHDOTG_KERNEL_DOWNLOAD); then install -T -m 0644 $(GLOHDOTG_KERNEL_DOWNLOAD) $(@D)/KoboRoot/opt/xcsoar/lib/kernel/uImage.glohd.otg; fi
	$(Q)if test -f $(AURA2_KERNEL_DOWNLOAD); then install -T -m 0644 $(AURA2_KERNEL_DOWNLOAD) $(@D)/KoboRoot/opt/xcsoar/lib/kernel/uImage.aura2; fi
	$(Q)if test -f $(AURA2OTG_KERNEL_DOWNLOAD); then install -T -m 0644 $(AURA2OTG_KERNEL_DOWNLOAD) $(@D)/KoboRoot/opt/xcsoar/lib/kernel/uImage.aura2.otg; fi
	$(Q)if test -f $(CLARAHD_DRIVERS_DOWNLOAD); then tar -xzf $(CLARAHD_DRIVERS_DOWNLOAD) -C $(@D)/KoboRoot/opt/xcsoar/lib; fi
	$(Q)install -m 0644 $(topdir)/kobo/inittab $(@D)/KoboRoot/etc
	$(Q)install -m 0644 $(topdir)/kobo/udev.rules $(@D)/KoboRoot/etc/udev/rules.d/99-xcsoar.rules
	$(Q)install -m 0644 $(BITSTREAM_VERA_FILES) $(@D)/KoboRoot/opt/xcsoar/share/fonts
	$(Q)fakeroot tar czfC $@ $(@D)/KoboRoot .

endif
