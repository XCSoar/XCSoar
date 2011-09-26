ifeq ($(TARGET),UNIX)

DESTDIR =
prefix = $(DESTDIR)/usr

install: all mo manual
	install -d -m 0755 $(prefix)/bin $(prefix)/share/doc/xcsoar
	install -m 0755 output/UNIX/bin/xcsoar output/UNIX/bin/vali-xcs $(prefix)/bin
	install -d -m 0755 $(patsubst %,$(prefix)/share/locale/%/LC_MESSAGES,$(LINGUAS))
	for i in $(LINGUAS); do \
		install -m 0644 output/po/$$i.mo $(prefix)/share/locale/$$i/LC_MESSAGES/xcsoar.mo; \
	done
	install -m 0644 output/manual/XCSoar-manual.pdf output/manual/XCSoar-developer-manual.pdf $(prefix)/share/doc/xcsoar

endif
