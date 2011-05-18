XGETTEXT = xgettext
MSGCAT = msgcat
MSGFMT = msgfmt
MSGMERGE = msgmerge

GETTEXT_PACKAGE = xcsoar
GETTEXT_SOURCES = $(XCSOAR_SOURCES) \
	$(DRIVER_SOURCES)
GETTEXT_DIALOGS = $(wildcard Data/Dialogs/*.xml)
GETTEXT_DIALOGS += $(wildcard Data/Dialogs/Infobox/*.xml)
GETTEXT_EVENTS = Data/Input/default.xci
PO_FILES = $(wildcard po/*.po)
MO_FILES = $(patsubst po/%.po,$(OUT)/po/%.mo,$(PO_FILES))
LINGUAS = $(patsubst po/%.po,%,$(PO_FILES))

ifeq ($(TARGET),UNIX)
ifeq ($(shell uname -s),Darwin)
GETTEXT_LDLIBS = /opt/local/lib/libintl.a /opt/local/lib/libiconv.a
endif
endif

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

$(OUT)/po/xml.pot: $(GETTEXT_DIALOGS) | $(OUT)/po/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/xml2po.pl $^ >$@.tmp
	$(Q)mv $@.tmp $@

$(OUT)/po/event.pot: $(GETTEXT_EVENTS) | $(OUT)/po/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/xci2po.pl $^ >$@.tmp
	$(Q)mv $@.tmp $@

po/$(GETTEXT_PACKAGE).pot: $(OUT)/po/cpp.pot $(OUT)/po/xml.pot $(OUT)/po/event.pot
	@$(NQ)echo "  GEN     $@"
	$(Q)$(MSGCAT) -o $@ $^

mo: $(MO_FILES)

update-po: po/$(GETTEXT_PACKAGE).pot
	$(Q)for i in $(PO_FILES); do $(MSGMERGE) -o $$i $$i po/$(GETTEXT_PACKAGE).pot; done

$(MO_FILES): $(OUT)/po/%.mo: po/%.po | $(OUT)/po/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(MSGFMT) --check -o $@ $<
