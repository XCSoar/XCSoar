KOBO_MENU_SOURCES = \
	$(SRC)/Formatter/HexColor.cpp \
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

$(eval $(call link-program,KoboMenu,KOBO_MENU))

ifeq ($(TARGET),UNIX)
OPTIONAL_OUTPUTS += $(KOBO_MENU_BIN)
endif
