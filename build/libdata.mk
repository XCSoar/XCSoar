# The data library contains data files such as the license file, the
# authors list and others

DATA_RESOURCES = \
	$(MO_FILES) \
	output/data/COPYING.gz \
	output/data/AUTHORS.gz \
	Data/other/egm96s.dem
DATA_SOURCES += $(foreach file,$(DATA_RESOURCES),$(DATA)/$(notdir $(file)).c)

define add-data-file
$(DATA)/$(notdir $(1)).c: $(1) $(topdir)/tools/BinToC.pl $(topdir)/tools/BinToC.pm | $$(DATA)/dirstamp
	@$$(NQ)echo "  GEN     $$@"
	$(Q)$(PERL) $(topdir)/tools/BinToC.pl $$< $$@
endef

$(foreach file,$(DATA_RESOURCES),$(eval $(call add-data-file,$(file))))

$(eval $(call link-library,libdata,DATA))
