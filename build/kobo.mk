KOBO_MENU_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Asset.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Hardware/CPU.cpp \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(SRC)/Hardware/DisplaySize.cpp \
	$(SRC)/Hardware/RotateDisplay.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/TerminalWindow.cpp \
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
	$(SRC)/Dialogs/TextEntry.cpp \
	$(SRC)/Dialogs/KnobTextEntry.cpp \
	$(SRC)/Dialogs/TouchTextEntry.cpp \
	$(SRC)/Dialogs/SimulatorPromptWindow.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(SRC)/Kobo/WPASupplicant.cpp \
	$(SRC)/Kobo/Model.cpp \
	$(SRC)/Kobo/System.cpp \
	$(SRC)/Kobo/Kernel.cpp \
	$(SRC)/Kobo/NetworkDialog.cpp \
	$(SRC)/Kobo/SystemDialog.cpp \
	$(SRC)/Kobo/ToolsDialog.cpp \
	$(SRC)/Kobo/WPASupplicant.cpp \
	$(SRC)/Kobo/WifiDialog.cpp \
	$(SRC)/Kobo/FakeSymbols.cpp \
	$(SRC)/Kobo/KoboMenu.cpp
KOBO_MENU_LDADD = $(FAKE_LIBS)
KOBO_MENU_DEPENDS = WIDGET FORM SCREEN EVENT RESOURCE IO ASYNC LIBNET OS THREAD MATH UTIL
KOBO_MENU_STRIP = y

$(eval $(call link-program,KoboMenu,KOBO_MENU))

ifeq ($(TARGET),UNIX)
OPTIONAL_OUTPUTS += $(KOBO_MENU_BIN)
endif

ifeq ($(TARGET_IS_KOBO),y)

KOBO_POWER_OFF_SOURCES = \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(SRC)/Hardware/RotateDisplay.cpp \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(SRC)/Hardware/DisplaySize.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Logger/FlightParser.cpp \
	$(SRC)/Renderer/FlightListRenderer.cpp \
	$(SRC)/FlightInfo.cpp \
	$(SRC)/Kobo/Model.cpp \
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

ifeq ($(USE_CROSSTOOL_NG),y)
  SYSROOT = $(shell $(CC) -print-sysroot)
else
  # from Debian package libc6-armhf-cross
  SYSROOT = /usr/arm-linux-gnueabihf
endif

# install our version of the system libraries in /opt/xcsoar/lib; this
# is necessary because:
# - we cannot link statically because we need NSS (name service
#   switch) modules
# - Kobo's stock glibc may be incompatible on some older firmware
#   versions
KOBO_SYS_LIB_NAMES = libc.so.6 libm.so.6 libpthread.so.0 librt.so.1 \
	libdl.so.2 \
	libresolv.so.2 libnss_dns.so.2 libnss_files.so.2 \
	ld-linux-armhf.so.3

KOBO_SYS_LIB_PATHS = $(addprefix $(THIRDPARTY_LIBS_ROOT)/lib/,$(KOBO_SYS_LIB_NAMES))

# from Debian package libgcc1-armhf-cross
KOBO_SYS_LIB_PATHS += $(SYSROOT)/lib/libgcc_s.so.1

KOBO_KERNEL_DIR = /opt/kobo/kernel

# /mnt/onboard/.kobo/KoboRoot.tgz is a file that is picked up by
# /etc/init.d/rcS, extracted to / on each boot; we can use it to
# install XCSoar
$(TARGET_OUTPUT_DIR)/KoboRoot.tgz: $(XCSOAR_BIN) \
	$(KOBO_MENU_BIN) $(KOBO_POWER_OFF_BIN) \
	$(BITSTREAM_VERA_FILES) \
	$(topdir)/kobo/inittab $(topdir)/kobo/rcS
	@$(NQ)echo "  TAR     $@"
	$(Q)rm -rf $(@D)/KoboRoot
	$(Q)install -m 0755 -d $(@D)/KoboRoot/etc $(@D)/KoboRoot/opt/xcsoar/bin $(@D)/KoboRoot/opt/xcsoar/lib/kernel $(@D)/KoboRoot/opt/xcsoar/share/fonts
	$(Q)install -m 0755 $(XCSOAR_BIN) $(KOBO_MENU_BIN) $(KOBO_POWER_OFF_BIN) $(topdir)/kobo/rcS $(@D)/KoboRoot/opt/xcsoar/bin
	$(Q)install --strip --strip-program=$(STRIP) -m 0755 $(KOBO_SYS_LIB_PATHS) $(@D)/KoboRoot/opt/xcsoar/lib
	$(Q)if test -f $(KOBO_KERNEL_DIR)/uImage.kobo; then install -m 0644 $(KOBO_KERNEL_DIR)/uImage.kobo $(@D)/KoboRoot/opt/xcsoar/lib/kernel; fi
	$(Q)if test -f $(KOBO_KERNEL_DIR)/uImage.otg; then install -m 0644 $(KOBO_KERNEL_DIR)/uImage.otg $(@D)/KoboRoot/opt/xcsoar/lib/kernel; fi
	$(Q)if test -f $(KOBO_KERNEL_DIR)/uImage.glohd; then install -m 0644 $(KOBO_KERNEL_DIR)/uImage.glohd $(@D)/KoboRoot/opt/xcsoar/lib/kernel; fi
	$(Q)if test -f $(KOBO_KERNEL_DIR)/uImage.glohd.otg; then install -m 0644 $(KOBO_KERNEL_DIR)/uImage.glohd.otg $(@D)/KoboRoot/opt/xcsoar/lib/kernel; fi
	$(Q)if test -f $(KOBO_KERNEL_DIR)/uImage.aura2; then install -m 0644 $(KOBO_KERNEL_DIR)/uImage.aura2 $(@D)/KoboRoot/opt/xcsoar/lib/kernel; fi
	$(Q)if test -f $(KOBO_KERNEL_DIR)/uImage.aura2.otg; then install -m 0644 $(KOBO_KERNEL_DIR)/uImage.aura2.otg $(@D)/KoboRoot/opt/xcsoar/lib/kernel; fi
	$(Q)install -m 0644 $(topdir)/kobo/inittab $(@D)/KoboRoot/etc
	$(Q)install -m 0644 $(BITSTREAM_VERA_FILES) $(@D)/KoboRoot/opt/xcsoar/share/fonts
	$(Q)fakeroot tar czfC $@ $(@D)/KoboRoot .

endif
