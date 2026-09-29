ifeq ($(TARGET),UNIX)

DESTDIR =
prefix = $(DESTDIR)/usr

install-mo: mo
	install -d -m 0755 $(patsubst %,$(prefix)/share/locale/%/LC_MESSAGES,$(LINGUAS))
	for i in $(LINGUAS); do \
		install -m 0644 $(OUT)/po/$$i.mo $(prefix)/share/locale/$$i/LC_MESSAGES/xcsoar.mo; \
	done

install-bin: all
	install -d -m 0755 $(prefix)/bin
	install -m 0755 $(TARGET_BIN_DIR)/xcsoar $(TARGET_BIN_DIR)/vali-xcs $(prefix)/bin

install-desktop:
	install -d -m 0755 $(prefix)/share/applications
	install -m 0644 $(topdir)/unix/xcsoar.desktop $(prefix)/share/applications/xcsoar.desktop
	install -d -m 0755 $(prefix)/share/icons/hicolor/scalable/apps
	install -m 0644 $(topdir)/Data/graphics/logo.svg $(prefix)/share/icons/hicolor/scalable/apps/xcsoar.svg

install-manual: manual
	install -d -m 0755 $(prefix)/share/doc/xcsoar
	install -m 0644 $(MANUAL_PDF) $(prefix)/share/doc/xcsoar

install: install-bin install-mo install-manual install-desktop

uninstall-mo:
	for i in $(LINGUAS); do \
		rm -f $(prefix)/share/locale/$$i/LC_MESSAGES/xcsoar.mo; \
	done

uninstall-bin:
	rm -f $(prefix)/bin/xcsoar $(prefix)/bin/vali-xcs

uninstall-desktop:
	rm -f $(prefix)/share/applications/xcsoar.desktop
	rm -f $(prefix)/share/icons/hicolor/scalable/apps/xcsoar.svg

uninstall-manual:
	rm -f $(prefix)/share/doc/xcsoar/$(notdir $(MANUAL_PDF))

uninstall: uninstall-bin uninstall-mo uninstall-manual uninstall-desktop

endif
