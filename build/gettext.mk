XGETTEXT = xgettext
GETTEXT_PACKAGE = xcsoar
GETTEXT_SOURCES = $(XCSOAR_SOURCES) \
	$(DRIVER_SOURCES)

po/$(GETTEXT_PACKAGE).pot:
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
	  $(GETTEXT_SOURCES)
