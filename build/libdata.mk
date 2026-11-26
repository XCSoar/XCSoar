# The data library contains data files such as the license file, the
# authors list and others

DATA_RESOURCES = \
	$(MO_FILES) \
	$(TEXT_COMPRESSED) \
	Data/other/egm96s.dem
DATA_SOURCES += $(foreach file,$(DATA_RESOURCES),$(DATA)/$(notdir $(file)).c)

define add-data-file
generate:: $(DATA)/$(notdir $(1)).c
$(DATA)/$(notdir $(1)).c: $(1) $(topdir)/tools/BinToC.pl $(topdir)/tools/BinToC.pm | $$(DATA)/dirstamp
	@$$(NQ)echo "  GEN     $$@"
	$(Q)$(PERL) $(topdir)/tools/BinToC.pl $$< $$@
endef

$(foreach file,$(DATA_RESOURCES),$(eval $(call add-data-file,$(file))))

# The data symbols are defined in top-level asm, which an LTO symbol
# table does not list; the linker would not extract them from libdata.a.
$(call SRC_TO_OBJ,$(DATA_SOURCES)): CFLAGS += -fno-lto

$(eval $(call link-library,libdata,DATA))
