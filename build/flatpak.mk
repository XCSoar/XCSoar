# Flatpak is a package format for the existing native Linux build, not a
# separate XCSoar target. Keep the default ID stable: it is the Flatpak
# update identity. Downstream forks may override it on the make command line.
FLATPAK_APP_ID ?= org.xcsoar.XCSoar
FLATPAK_REMOTE ?= flathub
FLATPAK_RUNTIME_REPO ?= https://dl.flathub.org/repo/flathub.flatpakrepo
FLATPAK_BUILDER ?= flatpak-builder
FLATPAK ?= flatpak
# Apple Container and other nested-container environments cannot mount
# rofiles-fuse.  Disabling it is slower but portable.  force-clean also makes
# a retry safe after flatpak-builder was interrupted before its first export.
# Override this (e.g. FLATPAK_BUILDER_OPTIONS=) to retain the local build dir.
FLATPAK_BUILDER_OPTIONS ?= --disable-rofiles-fuse --force-clean

FLATPAK_OUTPUT_DIR ?= $(OUT)/UNIX/flatpak
FLATPAK_BUILD_DIR = $(FLATPAK_OUTPUT_DIR)/build
FLATPAK_REPO_DIR = $(FLATPAK_OUTPUT_DIR)/repo
FLATPAK_MANIFEST = $(FLATPAK_OUTPUT_DIR)/$(FLATPAK_APP_ID).yml
FLATPAK_BUNDLE = $(FLATPAK_OUTPUT_DIR)/$(FLATPAK_APP_ID).flatpak
FLATPAK_SOURCE_TYPE ?= git

ifeq ($(FLATPAK_SOURCE_TYPE),git)
FLATPAK_SOURCE_COMMIT = $(shell git -C $(topdir) rev-parse HEAD)
endif

FLATPAK_TEMPLATES = \
	$(topdir)/flatpak/org.xcsoar.XCSoar.yml.in \
	$(topdir)/flatpak/org.xcsoar.XCSoar.desktop.in \
	$(topdir)/flatpak/org.xcsoar.XCSoar.metainfo.xml.in

$(FLATPAK_MANIFEST): $(FLATPAK_TEMPLATES) | $(FLATPAK_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  FLATPAK  manifest $(FLATPAK_APP_ID)"
	$(Q)sed -e 's/@FLATPAK_APP_ID@/$(FLATPAK_APP_ID)/g' \
		-e 's/@FLATPAK_SOURCE_TYPE@/$(FLATPAK_SOURCE_TYPE)/g' \
		-e 's/@FLATPAK_SOURCE_COMMIT@/commit: $(FLATPAK_SOURCE_COMMIT)/g' \
		-e '/^[[:space:]]*commit: *$$/d' \
		-e 's|@FLATPAK_SOURCE_ROOT@|../../..|g' \
		$(topdir)/flatpak/org.xcsoar.XCSoar.yml.in > $@

.PHONY: flatpak flatpak-manifest flatpak-clean flatpak-install \
	flatpak-install-metadata

flatpak-manifest: $(FLATPAK_MANIFEST)

flatpak: $(FLATPAK_MANIFEST)
	@$(NQ)echo "  FLATPAK  $(FLATPAK_BUNDLE)"
	$(Q)$(FLATPAK_BUILDER) $(FLATPAK_BUILDER_OPTIONS) \
		--install-deps-from=$(FLATPAK_REMOTE) \
		--repo=$(FLATPAK_REPO_DIR) \
		$(FLATPAK_BUILD_DIR) $(FLATPAK_MANIFEST)
	$(Q)$(FLATPAK) build-bundle $(FLATPAK_REPO_DIR) $(FLATPAK_BUNDLE) \
		$(FLATPAK_APP_ID) --runtime-repo=$(FLATPAK_RUNTIME_REPO)

# This target is run by flatpak-builder inside the Flatpak SDK.  Keep all
# XCSoar source preparation and installation in the existing Make system;
# the manifest only describes the sandbox and calls this target.
flatpak-install:
	$(Q)$(MAKE) $(BOOST_UNTAR_STAMP)
	$(Q)$(MAKE) install-bin install-mo
	$(Q)$(MAKE) flatpak-install-metadata

flatpak-install-metadata:
	install -Dm644 flatpak/org.xcsoar.XCSoar.desktop.in \
		$(prefix)/share/applications/$(FLATPAK_APP_ID).desktop
	sed -i 's/@FLATPAK_APP_ID@/$(FLATPAK_APP_ID)/g' \
		$(prefix)/share/applications/$(FLATPAK_APP_ID).desktop
	install -Dm644 flatpak/org.xcsoar.XCSoar.metainfo.xml.in \
		$(prefix)/share/metainfo/$(FLATPAK_APP_ID).metainfo.xml
	sed -i 's/@FLATPAK_APP_ID@/$(FLATPAK_APP_ID)/g' \
		$(prefix)/share/metainfo/$(FLATPAK_APP_ID).metainfo.xml
	install -Dm644 Data/graphics/logo.svg \
		$(prefix)/share/icons/hicolor/scalable/apps/$(FLATPAK_APP_ID).svg

flatpak-clean:
	@$(NQ)echo "cleaning Flatpak output"
	$(Q)rm -rf $(FLATPAK_OUTPUT_DIR)
