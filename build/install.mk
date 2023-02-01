ifeq ($(TARGET),UNIX)

DESTDIR =
prefix = $(DESTDIR)/usr

install-mo: mo
	install -d -m 0755 $(patsubst %,$(prefix)/share/locale/%/LC_MESSAGES,$(LINGUAS))
	for i in $(LINGUAS); do \
		install -m 0644 $(OUT)/po/$$i.mo $(prefix)/share/locale/$$i/LC_MESSAGES/$(PROGRAM_NAME_LC).mo; \
	done

install-bin: all
	@$(NQ)echo "  INSTALL   Passiert hier irgend etwas????"
	install -d -m 0755 $(prefix)/bin
	install -m 0755 $(TARGET_BIN_DIR)/$(PROGRAM_NAME) $(TARGET_BIN_DIR)/vali-xcs $(prefix)/bin

install-manual: manual
	install -d -m 0755 $(prefix)/share/doc/$(PROGRAM_NAME)
	install -m 0644 $(MANUAL_PDF) $(prefix)/share/doc/$(PROGRAM_NAME)

install: install-bin install-mo install-manual

endif
