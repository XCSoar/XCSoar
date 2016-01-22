# Generate .po and .mo files.  For the gettext library (aka libintl),
# see libintl.mk.

XGETTEXT = xgettext
MSGCAT = msgcat
MSGFMT = msgfmt
MSGMERGE = msgmerge

GETTEXT_PACKAGE = xcsoar
GETTEXT_SOURCES = $(XCSOAR_SOURCES) \
	$(wildcard $(SRC)/Dialogs/Device/Vega/*Parameters.hpp) \
	$(SRC)/Weather/Rasp/RaspStore.cpp
GETTEXT_EVENTS = Data/Input/default.xci

$(OUT)/po/cpp.pot: $(GETTEXT_SOURCES) | $(OUT)/po/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(XGETTEXT) --default-domain=$(GETTEXT_PACKAGE) \
	  --add-comments --keyword=_ --keyword=N_ \
	  --from-code=utf-8 \
	  --keyword=C_:1c,2 \
	  --keyword=NC_:1c,2 \
	  --flag=N_:1:no-c-format \
	  --flag=C_:2:no-c-format \
	  --flag=NC_:2:no-c-format \
	  --output=$@ \
	  --force-po \
	  $^

$(OUT)/po/event.pot: $(GETTEXT_EVENTS) | $(OUT)/po/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/xci2po.pl $^ >$@.tmp
	$(Q)mv $@.tmp $@

po/$(GETTEXT_PACKAGE).pot: $(OUT)/po/cpp.pot $(OUT)/po/event.pot
	@$(NQ)echo "  GEN     $@"
	$(Q)$(MSGCAT) -o $@ $^

mo: $(MO_FILES)

update-po: po/$(GETTEXT_PACKAGE).pot
	$(Q)for i in $(PO_FILES); do $(MSGMERGE) -o $$i $$i po/$(GETTEXT_PACKAGE).pot; done

$(MO_FILES): $(OUT)/po/%.mo: po/%.po | $(OUT)/po/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(MSGFMT) --check -o $@ $<
