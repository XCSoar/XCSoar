# Build rules for the portable screen library

FORM_SRC_DIR = $(SRC)/Form

FORM_SOURCES = \
	$(SRC)/UIUtil/KineticManager.cpp \
	$(SRC)/Renderer/ButtonRenderer.cpp \
	$(SRC)/Renderer/SymbolRenderer.cpp \
	$(FORM_SRC_DIR)/Internal.cpp \
	$(FORM_SRC_DIR)/Control.cpp \
	$(FORM_SRC_DIR)/Panel.cpp \
	$(FORM_SRC_DIR)/SubForm.cpp \
	$(FORM_SRC_DIR)/Form.cpp \
	$(FORM_SRC_DIR)/Button.cpp \
	$(FORM_SRC_DIR)/ButtonPanel.cpp \
	$(FORM_SRC_DIR)/CheckBox.cpp \
	$(FORM_SRC_DIR)/SymbolButton.cpp \
	$(FORM_SRC_DIR)/Frame.cpp \
	$(FORM_SRC_DIR)/Draw.cpp \
	$(FORM_SRC_DIR)/List.cpp \
	$(FORM_SRC_DIR)/ScrollBar.cpp \
	$(FORM_SRC_DIR)/Edit.cpp \
	$(FORM_SRC_DIR)/DigitEntry.cpp \
	$(FORM_SRC_DIR)/Tabbed.cpp \
	$(FORM_SRC_DIR)/TabBar.cpp \
	$(FORM_SRC_DIR)/TabDisplay.cpp \
	$(FORM_SRC_DIR)/TabMenu.cpp \
	$(FORM_SRC_DIR)/TabMenuDisplay.cpp \
	$(FORM_SRC_DIR)/CharacterButton.cpp \
	$(FORM_SRC_DIR)/UnitUtil.cpp \
	$(FORM_SRC_DIR)/GridView.cpp \
	$(FORM_SRC_DIR)/CustomButton.cpp \
	$(FORM_SRC_DIR)/HLine.cpp \
	$(FORM_SRC_DIR)/Util.cpp

FORM_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

$(eval $(call link-library,form,FORM))
