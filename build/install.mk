ifeq ($(TARGET),UNIX)

DESTDIR =
prefix = $(DESTDIR)/usr

install: all mo manual
	install -d -m 0755 $(prefix)/bin $(prefix)/share/doc/xcsoar
	install -m 0755 $(TARGET_BIN_DIR)/xcsoar $(TARGET_BIN_DIR)/vali-xcs $(prefix)/bin
	install -d -m 0755 $(patsubst %,$(prefix)/share/locale/%/LC_MESSAGES,$(LINGUAS))
	for i in $(LINGUAS); do \
		install -m 0644 $(OUT)/po/$$i.mo $(prefix)/share/locale/$$i/LC_MESSAGES/xcsoar.mo; \
	done
	install -m 0644 $(MANUAL_PDF) $(prefix)/share/doc/xcsoar

endif
