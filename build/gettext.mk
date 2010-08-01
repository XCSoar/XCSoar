XGETTEXT = xgettext
GETTEXT_PACKAGE = xcsoar
GETTEXT_SOURCES = $(XCSOAR_SOURCES) \
	$(DRIVER_SOURCES)
GETTEXT_DIALOGS = $(wildcard Data/Dialogs/*.xml)

$(OUT)/po/cpp.pot: $(GETTEXT_SOURCES) | $(OUT)/po/dirstamp
	$(XGETTEXT) --default-domain=$(GETTEXT_PACKAGE) \
	  --add-comments --keyword=_ --keyword=N_ \
	  --from-code=utf-8 \
	  --keyword=C_:1c,2 \
	  --keyword=NC_:1c,2 \
	  --flag=N_:1:pass-c-format \
	  --flag=C_:2:pass-c-format \
	  --flag=NC_:2:pass-c-format \
	  --output=$@ \
	  --force-po \
	  $^

$(OUT)/po/xml.pot: $(GETTEXT_DIALOGS) | $(OUT)/po/dirstamp
	$(Q)$(PERL) $(topdir)/tools/xml2po.pl $^ >$@.tmp
	$(Q)mv $@.tmp $@

po/$(GETTEXT_PACKAGE).pot: $(OUT)/po/cpp.pot $(OUT)/po/xml.pot
	@$(NQ)echo "  GEN     $@"
	$(Q)msgcat -o $@ $^
