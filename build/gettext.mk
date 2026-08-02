# Generate .po and .mo files.  For the gettext library (aka libintl),
# see libintl.mk.

XGETTEXT = xgettext
MSGCAT = msgcat
MSGFMT = msgfmt
MSGMERGE = msgmerge
MSGATTRIB = msgattrib
GETTEXT_NO_WRAP = --no-wrap
# Omit #: file:line references — they churn on every source edit and are
# not required at runtime. update_po_low_churn strips them from .po files
# while preserving msgid/msgstr wrapping on untouched translations.
GETTEXT_NO_LOCATION = --no-location

GETTEXT_PACKAGE = xcsoar
GETTEXT_SOURCES = $(XCSOAR_SOURCES) \
	$(LIBINFOBOX_SOURCES) \
	$(LIBMAPWINDOW_SOURCES) \
	$(LIBCOMPUTER_SOURCES) \
	$(wildcard $(SRC)/Dialogs/Device/Vega/*Parameters.hpp) \
	$(SRC)/Language/FormatText.hpp \
	$(SRC)/Weather/Rasp/RaspStore.cpp
GETTEXT_EVENTS = Data/Input/default.xci

$(OUT)/po/cpp.pot: $(GETTEXT_SOURCES) | $(OUT)/po/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(XGETTEXT) --default-domain=$(GETTEXT_PACKAGE) \
	  --package-name=$(GETTEXT_PACKAGE) \
	  --add-comments --keyword=_ --keyword=N_ \
	  --from-code=utf-8 \
	  $(GETTEXT_NO_WRAP) \
	  $(GETTEXT_NO_LOCATION) \
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
	$(Q)$(MSGCAT) $(GETTEXT_NO_WRAP) $(GETTEXT_NO_LOCATION) -o $@ $^

mo: $(MO_FILES)

update-po: po/$(GETTEXT_PACKAGE).pot
	$(Q)for i in $(PO_FILES); do \
	  tmp=$$(mktemp); \
	  $(MSGMERGE) $(GETTEXT_NO_WRAP) $(GETTEXT_NO_LOCATION) --previous \
	    -o $$tmp $$i po/$(GETTEXT_PACKAGE).pot; \
	  $(MSGATTRIB) --clear-fuzzy --empty --no-obsolete --clear-previous \
	    $(GETTEXT_NO_WRAP) $(GETTEXT_NO_LOCATION) -o $$tmp $$tmp; \
	  $(PYTHON) $(topdir)/tools/update_po_low_churn.py \
	    --original $$i --normalized $$tmp; \
	  rm -f $$tmp; \
	done

$(MO_FILES): $(OUT)/po/%.mo: po/%.po | $(OUT)/po/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(MSGFMT) --check -o $@ $<
