INCLUDES += -I$(OUT)/include

PERL = perl
RANDOM_NUMBER := $(shell od -vAn -N4 -tu4 < /dev/urandom| tr -d ' ')

$(OUT)/include/MathTables.h: $(HOST_OUTPUT_DIR)/tools/GenerateSineTables$(HOST_EXEEXT) | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	# Generate into a temporary file to avoid leaving a truncated/empty header on interruption
	$(Q)$(HOST_OUTPUT_DIR)/tools/GenerateSineTables$(HOST_EXEEXT) >$@.$(RANDOM_NUMBER).tmp
	# Basic validation: file must be non-empty and contain an expected token.
	@if [ ! -s "$@.$(RANDOM_NUMBER).tmp" ]; then \
	  echo "Generation failed: $@.$(RANDOM_NUMBER).tmp is empty" >&2; \
	  rm -f "$@.$(RANDOM_NUMBER).tmp"; \
	  exit 1; \
	fi
	@if ! grep -q 'THERMALRECENCY{' "$@.$(RANDOM_NUMBER).tmp"; then \
	  echo "Generation failed: expected token THERMALRECENCY{ not found in $@.$(RANDOM_NUMBER).tmp" >&2; \
	  rm -f "$@.$(RANDOM_NUMBER).tmp"; \
	  exit 1; \
	fi
	@mv $@.$(RANDOM_NUMBER).tmp $@

$(call SRC_TO_OBJ,$(SRC)/Math/FastMath.cpp): $(OUT)/include/MathTables.h
$(call SRC_TO_OBJ,$(SRC)/Math/FastTrig.cpp): $(OUT)/include/MathTables.h
$(call SRC_TO_OBJ,$(SRC)/Computer/ThermalRecency.cpp): $(OUT)/include/MathTables.h

$(OUT)/include/InputEvents_Text2Event.cpp: $(SRC)/Input/InputEvents.hpp \
	$(topdir)/tools/Text2Event.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2Event.pl $< >$@.$(RANDOM_NUMBER).tmp
	@mv $@.$(RANDOM_NUMBER).tmp $@

$(OUT)/include/InputEvents_Text2GCE.cpp: $(SRC)/Input/InputQueue.hpp \
	$(topdir)/tools/Text2GCE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2GCE.pl $< >$@.$(RANDOM_NUMBER).tmp
	@mv $@.$(RANDOM_NUMBER).tmp $@

$(OUT)/include/InputEvents_Text2NE.cpp: $(SRC)/Input/InputQueue.hpp \
	$(topdir)/tools/Text2NE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2NE.pl $< >$@.$(RANDOM_NUMBER).tmp
	@mv $@.$(RANDOM_NUMBER).tmp $@

$(OUT)/include/InputEvents_Char2GCE.cpp: $(SRC)/Input/InputQueue.hpp \
	$(topdir)/tools/Char2GCE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Char2GCE.pl $< >$@.$(RANDOM_NUMBER).tmp
	@mv $@.$(RANDOM_NUMBER).tmp $@

$(OUT)/include/InputEvents_Char2NE.cpp: $(SRC)/Input/InputQueue.hpp \
	$(topdir)/tools/Char2NE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Char2NE.pl $< >$@.$(RANDOM_NUMBER).tmp
	@mv $@.$(RANDOM_NUMBER).tmp $@

XCI_LIST = default
XCI_HEADERS = $(patsubst %,$(OUT)/include/InputEvents_%.cpp,$(XCI_LIST))

$(OUT)/include/InputEvents_default.cpp: $(topdir)/Data/Input/default.xci \
	$(topdir)/tools/xci2cpp.pl \
	| $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/xci2cpp.pl $< >$@.$(RANDOM_NUMBER).tmp
	@mv $@.$(RANDOM_NUMBER).tmp $@

$(call SRC_TO_OBJ,$(SRC)/Input/InputDefaults.cpp): $(XCI_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Input/InputLookup.cpp): $(OUT)/include/InputEvents_Text2Event.cpp $(OUT)/include/InputEvents_Text2GCE.cpp $(OUT)/include/InputEvents_Text2NE.cpp

$(call SRC_TO_OBJ,$(SRC)/lua/InputEvent.cpp): $(OUT)/include/InputEvents_Char2GCE.cpp $(OUT)/include/InputEvents_Char2NE.cpp

$(OUT)/include/Status_defaults.cpp: Data/Status/default.xcs \
	tools/xcs2cpp.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/xcs2cpp.pl $< >$@.$(RANDOM_NUMBER).tmp
	@mv $@.$(RANDOM_NUMBER).tmp $@

SM_OBJ = $(call SRC_TO_OBJ,$(SRC)/StatusMessage.cpp)
$(SM_OBJ): $(OUT)/include/Status_defaults.cpp

generate:: $(OUT)/include/MathTables.h $(XCI_HEADERS) \
	$(OUT)/include/Status_defaults.cpp \
	$(OUT)/include/InputEvents_Text2Event.cpp $(OUT)/include/InputEvents_Text2GCE.cpp $(OUT)/include/InputEvents_Text2NE.cpp \
	$(OUT)/include/InputEvents_Char2GCE.cpp $(OUT)/include/InputEvents_Char2NE.cpp

# UNIX resources

ifeq ($(USE_WIN32_RESOURCES),n)

ifeq ($(TARGET_IS_ANDROID),n)

$(TARGET_OUTPUT_DIR)/include/resource_data.h: $(TARGET_OUTPUT_DIR)/resources.txt \
	$(RESOURCE_FILES) \
	tools/GenerateResources.pl | $(TARGET_OUTPUT_DIR)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/GenerateResources.pl $< >$@.$(RANDOM_NUMBER).tmp
	@mv $@.$(RANDOM_NUMBER).tmp $@

$(call SRC_TO_OBJ,$(SRC)/ResourceLoader.cpp): $(TARGET_OUTPUT_DIR)/include/resource_data.h

generate:: $(TARGET_OUTPUT_DIR)/include/resource_data.h

endif # !TARGET_IS_ANDROID

endif
