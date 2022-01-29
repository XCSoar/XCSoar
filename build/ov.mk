OV_MENU_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Asset.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
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
	$(SRC)/Dialogs/LockScreen.cpp \
	$(SRC)/Dialogs/TextEntry.cpp \
	$(SRC)/Dialogs/KnobTextEntry.cpp \
	$(SRC)/Dialogs/TouchTextEntry.cpp \
	$(SRC)/Dialogs/ProcessDialog.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(SRC)/Kobo/FakeSymbols.cpp \
	$(SRC)/OV/OpenVarioMenu.cpp
OV_MENU_DEPENDS = WIDGET FORM DATA_FIELD SCREEN EVENT RESOURCE ASYNC LIBNET OS IO THREAD TIME MATH UTIL
OV_MENU_STRIP = y

$(eval $(call link-program,OpenVarioMenu,OV_MENU))

ifeq ($(TARGET),UNIX)
OPTIONAL_OUTPUTS += $(OV_MENU_BIN)
endif
