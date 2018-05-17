INCLUDES += -I$(OUT)/include

PERL = perl

$(OUT)/include/MathTables.h: $(HOST_OUTPUT_DIR)/tools/GenerateSineTables$(HOST_EXEEXT) | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(HOST_OUTPUT_DIR)/tools/GenerateSineTables$(HOST_EXEEXT) >$@

$(call SRC_TO_OBJ,$(SRC)/Math/FastMath.cpp): $(OUT)/include/MathTables.h
$(call SRC_TO_OBJ,$(SRC)/Math/FastTrig.cpp): $(OUT)/include/MathTables.h

$(OUT)/include/InputEvents_Text2Event.cpp: $(SRC)/Input/InputEvents.hpp \
	$(topdir)/tools/Text2Event.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2Event.pl $< >$@.tmp
	@mv $@.tmp $@

$(OUT)/include/InputEvents_Text2GCE.cpp: $(SRC)/Input/InputQueue.hpp \
	$(topdir)/tools/Text2GCE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2GCE.pl $< >$@.tmp
	@mv $@.tmp $@

$(OUT)/include/InputEvents_Text2NE.cpp: $(SRC)/Input/InputQueue.hpp \
	$(topdir)/tools/Text2NE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2NE.pl $< >$@.tmp
	@mv $@.tmp $@

$(OUT)/include/InputEvents_Char2GCE.cpp: $(SRC)/Input/InputQueue.hpp \
	$(topdir)/tools/Char2GCE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Char2GCE.pl $< >$@.tmp
	@mv $@.tmp $@

$(OUT)/include/InputEvents_Char2NE.cpp: $(SRC)/Input/InputQueue.hpp \
	$(topdir)/tools/Char2NE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Char2NE.pl $< >$@.tmp
	@mv $@.tmp $@

XCI_LIST = default
XCI_HEADERS = $(patsubst %,$(OUT)/include/InputEvents_%.cpp,$(XCI_LIST))

$(OUT)/include/InputEvents_default.cpp: $(topdir)/Data/Input/default.xci \
	$(topdir)/tools/xci2cpp.pl \
	| $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/xci2cpp.pl $< >$@.tmp
	@mv $@.tmp $@

$(call SRC_TO_OBJ,$(SRC)/Input/InputDefaults.cpp): $(XCI_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Input/InputLookup.cpp): $(OUT)/include/InputEvents_Text2Event.cpp $(OUT)/include/InputEvents_Text2GCE.cpp $(OUT)/include/InputEvents_Text2NE.cpp

$(call SRC_TO_OBJ,$(SRC)/Lua/InputEvent.cpp): $(OUT)/include/InputEvents_Char2GCE.cpp $(OUT)/include/InputEvents_Char2NE.cpp

$(OUT)/include/Status_defaults.cpp: Data/Status/default.xcs \
	tools/xcs2cpp.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/xcs2cpp.pl $< >$@.tmp
	@mv $@.tmp $@

SM_OBJ = $(call SRC_TO_OBJ,$(SRC)/StatusMessage.cpp)
$(SM_OBJ): $(OUT)/include/Status_defaults.cpp

# UNIX resources

ifeq ($(USE_WIN32_RESOURCES),n)

$(TARGET_OUTPUT_DIR)/XCSoar.rc: Data/XCSoar.rc $(OUT)/include/resource.h | $(TARGET_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  CPP     $@"
	$(Q)cat $< | $(HOSTCC) -E -o $@ -I$(OUT)/include $(TARGET_CPPFLAGS) $(OPENGL_CPPFLAGS) -

$(TARGET_OUTPUT_DIR)/include/resource_data.h: $(TARGET_OUTPUT_DIR)/XCSoar.rc \
	$(RESOURCE_FILES) \
	tools/GenerateResources.pl | $(TARGET_OUTPUT_DIR)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/GenerateResources.pl $< >$@.tmp
	@mv $@.tmp $@

$(TARGET_OUTPUT_DIR)/XCSoar-drawable.rc: Data/XCSoar.rc $(OUT)/include/resource.h | $(TARGET_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  CPP     $@"
	$(Q)cat $< | $(HOSTCC) -E -o $@ $< -I$(OUT)/include $(TARGET_CPPFLAGS) -DANDROID_DRAWABLE -

$(TARGET_OUTPUT_DIR)/include/android_drawable.h: $(TARGET_OUTPUT_DIR)/XCSoar-drawable.rc \
	$(RESOURCE_FILES) \
	tools/GenerateAndroidResources.pl | $(TARGET_OUTPUT_DIR)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/GenerateAndroidResources.pl $< >$@.tmp
	@mv $@.tmp $@

$(call SRC_TO_OBJ,$(SRC)/ResourceLoader.cpp): $(TARGET_OUTPUT_DIR)/include/resource_data.h

ifeq ($(TARGET),ANDROID)
$(call SRC_TO_OBJ,$(SRC)/Screen/Android/Bitmap.cpp): $(TARGET_OUTPUT_DIR)/include/android_drawable.h
endif

endif
