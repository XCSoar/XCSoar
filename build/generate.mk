INCLUDES += -I$(OUT)/include

PERL = perl

$(OUT)/include/MathTables.h: $(HOST_OUTPUT_DIR)/tools/GenerateSineTables$(HOST_EXEEXT) | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(HOST_OUTPUT_DIR)/tools/GenerateSineTables$(HOST_EXEEXT) >$@

$(call SRC_TO_OBJ,$(SRC)/Math/FastMath.c): $(OUT)/include/MathTables.h

$(OUT)/include/InputEvents_Text2Event.cpp: $(SRC)/InputEvents.hpp \
	$(topdir)/tools/Text2Event.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2Event.pl $< >$@.tmp
	@mv $@.tmp $@

$(OUT)/include/InputEvents_Text2GCE.cpp: $(SRC)/InputEvents.hpp \
	$(topdir)/tools/Text2GCE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2GCE.pl $< >$@.tmp
	@mv $@.tmp $@

$(OUT)/include/InputEvents_Text2NE.cpp: $(SRC)/InputEvents.hpp \
	$(topdir)/tools/Text2NE.pl | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/Text2NE.pl $< >$@.tmp
	@mv $@.tmp $@

XCI_LIST = altair default
XCI_HEADERS = $(patsubst %,$(OUT)/include/InputEvents_%.cpp,$(XCI_LIST))

$(OUT)/include/InputEvents_default.cpp: $(topdir)/Data/Input/default.xci \
	$(topdir)/tools/xci2cpp.pl \
	$(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/xci2cpp.pl $< >$@.tmp
	@mv $@.tmp $@

$(OUT)/include/InputEvents_altair.cpp: $(topdir)/Data/Input/altair.xci \
	$(topdir)/Data/Input/default.xci \
	$(topdir)/tools/xci2cpp.pl \
	$(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/xci2cpp.pl $(topdir)/Data/Input/default.xci $(topdir)/Data/Input/altair.xci >$@.tmp
	@mv $@.tmp $@

T2E_OBJ = $(call SRC_TO_OBJ,$(SRC)/InputEvents.cpp)
$(T2E_OBJ): $(XCI_HEADERS) $(OUT)/include/InputEvents_Text2Event.cpp $(OUT)/include/InputEvents_Text2GCE.cpp $(OUT)/include/InputEvents_Text2NE.cpp

$(OUT)/include/Status_defaults.cpp: Data/Status/default.xcs \
	tools/xcs2cpp.pl $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/xcs2cpp.pl $< >$@.tmp
	@mv $@.tmp $@

SM_OBJ = $(call SRC_TO_OBJ,$(SRC)/StatusMessage.cpp)
$(SM_OBJ): $(OUT)/include/Status_defaults.cpp

# UNIX resources

ifeq ($(HAVE_WIN32),n)

$(TARGET_OUTPUT_DIR)/XCSoar.rc: Data/XCSoar.rc src/resource.h | $(TARGET_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  CPP     $@"
	$(Q)$(HOSTCPP) -o $@ $< -I$(SRC) $(TARGET_CPPFLAGS)

$(TARGET_OUTPUT_DIR)/include/resource_data.h: $(TARGET_OUTPUT_DIR)/XCSoar.rc \
	$(RESOURCE_FILES) \
	tools/GenerateResources.pl | $(TARGET_OUTPUT_DIR)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/GenerateResources.pl $< >$@.tmp
	@mv $@.tmp $@

$(TARGET_OUTPUT_DIR)/XCSoar-drawable.rc: Data/XCSoar.rc src/resource.h | $(TARGET_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  CPP     $@"
	$(Q)$(HOSTCPP) -o $@ $< -I$(SRC) $(TARGET_CPPFLAGS) -DANDROID_DRAWABLE

$(TARGET_OUTPUT_DIR)/include/android_drawable.h: $(TARGET_OUTPUT_DIR)/XCSoar-drawable.rc \
	$(RESOURCE_FILES) \
	tools/GenerateAndroidResources.pl | $(TARGET_OUTPUT_DIR)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/GenerateAndroidResources.pl $< >$@.tmp
	@mv $@.tmp $@

$(TARGET_OUTPUT_DIR)/$(SRC)/ResourceLoader.o: $(TARGET_OUTPUT_DIR)/include/resource_data.h

ifeq ($(TARGET),ANDROID)
$(TARGET_OUTPUT_DIR)/$(SRC)/Screen/OpenGL/Bitmap.o: $(TARGET_OUTPUT_DIR)/include/android_drawable.h
endif

endif
