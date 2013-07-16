KOBO_MENU_SOURCES = \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Hardware/CPU.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Screen/TerminalWindow.cpp \
	$(SRC)/Look/TerminalLook.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(SRC)/Kobo/System.cpp \
	$(SRC)/Kobo/KoboMenu.cpp
KOBO_MENU_LDADD = $(FAKE_LIBS)
KOBO_MENU_DEPENDS = FORM SCREEN EVENT ASYNC OS THREAD MATH UTIL
KOBO_MENU_STRIP = y

$(eval $(call link-program,KoboMenu,KOBO_MENU))

ifeq ($(TARGET),UNIX)
OPTIONAL_OUTPUTS += $(KOBO_MENU_BIN)
endif

ifeq ($(TARGET_IS_KOBO),y)

BITSTREAM_VERA_DIR ?= /usr/share/fonts/truetype/ttf-bitstream-vera
BITSTREAM_VERA_NAMES = Vera VeraBd VeraMono
BITSTREAM_VERA_FILES = $(patsubst %,$(BITSTREAM_VERA_DIR)/%.ttf,$(BITSTREAM_VERA_NAMES))

# /mnt/onboard/.kobo/KoboRoot.tgz is a file that is picked up by
# /etc/init.d/rcS, extracted to / on each boot; we can use it to
# install XCSoar
$(TARGET_OUTPUT_DIR)/KoboRoot.tgz: $(KOBO_MENU_BIN) $(XCSOAR_BIN) \
	$(BITSTREAM_VERA_FILES) \
	$(topdir)/kobo/inittab $(topdir)/kobo/rcS
	@$(NQ)echo "  TAR     $@"
	$(Q)rm -rf $(@D)/KoboRoot
	$(Q)install -m 0755 -d $(@D)/KoboRoot/etc $(@D)/KoboRoot/opt/xcsoar/bin $(@D)/KoboRoot/opt/xcsoar/share/fonts
	$(Q)install -m 0755 $(KOBO_MENU_BIN) $(XCSOAR_BIN) $(topdir)/kobo/rcS $(@D)/KoboRoot/opt/xcsoar/bin
	$(Q)install -m 0644 $(topdir)/kobo/inittab $(@D)/KoboRoot/etc
	$(Q)install -m 0644 $(BITSTREAM_VERA_FILES) $(@D)/KoboRoot/opt/xcsoar/share/fonts
	$(Q)fakeroot tar czfC $@ $(@D)/KoboRoot .

endif
