# Build rules for the portable screen library

FORM_SRC_DIR = $(SRC)/Form

FORM_SOURCES = \
	$(SRC)/KineticManager.cpp \
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
	$(FORM_SRC_DIR)/Widget.cpp \
	$(FORM_SRC_DIR)/WindowWidget.cpp \
	$(FORM_SRC_DIR)/Tabbed.cpp \
	$(FORM_SRC_DIR)/TabBar.cpp \
	$(FORM_SRC_DIR)/TabMenu.cpp \
	$(FORM_SRC_DIR)/Keyboard.cpp \
	$(FORM_SRC_DIR)/UnitUtil.cpp \
	$(FORM_SRC_DIR)/ProfileUtil.cpp \
	$(FORM_SRC_DIR)/Util.cpp

FORM_OBJS = $(call SRC_TO_OBJ,$(FORM_SOURCES))

FORM_LIBS = $(TARGET_OUTPUT_DIR)/form.a

$(FORM_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(FORM_LIBS): $(FORM_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
