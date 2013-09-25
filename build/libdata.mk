# The data library contains data files such as the license file, the
# authors list and others

DATA_SOURCES = \
	$(MO_FILES) \
	output/data/COPYING.gz \
	output/data/AUTHORS.gz \
	Data/other/egm96s.dem
DATA_OBJS = $(foreach file,$(DATA_SOURCES),$(TARGET_OUTPUT_DIR)/data/$(notdir $(file)))

define add-data-file
$$(TARGET_OUTPUT_DIR)/data/$(notdir $(1)): $(1) | $$(TARGET_OUTPUT_DIR)/data/dirstamp
	@$$(NQ)echo "  GEN     $$@"
	$$(Q)cd $$(<D) && $$(LD) -r -b binary -o $$(abspath $$@) $$(notdir $$<)
endef

$(foreach file,$(DATA_SOURCES),$(eval $(call add-data-file,$(file))))

DATA_LDADD = $(TARGET_OUTPUT_DIR)/libdata.a

$(DATA_LDADD): $(DATA_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
