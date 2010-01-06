INCLUDES += -I$(OUT)/include

PERL = perl

$(OUT)/include/InputEvents_Text2Event.cpp: $(SRC)/InputEvents.h \
	$(topdir)/Data/Input/h2cpp.pl $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/Data/Input/h2cpp.pl $< >$@.tmp
	@mv $@.tmp $@

XCI_LIST = pc altair pna fivv default
XCI_HEADERS = $(patsubst %,$(OUT)/include/InputEvents_%.cpp,$(XCI_LIST))

$(XCI_HEADERS): $(OUT)/include/InputEvents_%.cpp: \
	$(topdir)/Data/Input/%.xci $(topdir)/Data/Input/xci2cpp.pl \
	$(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/Data/Input/xci2cpp.pl $< >$@.tmp
	@mv $@.tmp $@

T2E_OBJ = $(call SRC_TO_OBJ,$(SRC)/InputEvents.cpp)
$(T2E_OBJ) $(T2E_OBJ:$(OBJ_SUFFIX)=-Simulator$(OBJ_SUFFIX)): \
	$(XCI_HEADERS) \
	$(OUT)/include/InputEvents_Text2Event.cpp

$(OUT)/include/Status_defaults.cpp: Data/Status/default.xcs \
	Data/Status/xcs2cpp.pl $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) Data/Status/xcs2cpp.pl $< >$@.tmp
	@mv $@.tmp $@

SM_OBJ = $(call SRC_TO_OBJ,$(SRC)/StatusMessage.cpp)
$(SM_OBJ) $(SM_OBJ:$(OBJ_SUFFIX)=-Simulator$(OBJ_SUFFIX)): \
	$(OUT)/include/Status_defaults.cpp
