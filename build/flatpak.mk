# Flatpak is a package format for the existing native Linux build, not a
# separate XCSoar target. Keep the default ID stable: it is the Flatpak
# update identity. Downstream forks may override it on the make command line.
FLATPAK_APP_ID ?= org.xcsoar.XCSoar
FLATPAK_REMOTE ?= flathub
FLATPAK_RUNTIME_REPO ?= https://dl.flathub.org/repo/flathub.flatpakrepo
FLATPAK_BUILDER ?= flatpak-builder
FLATPAK ?= flatpak
# Apple Container and other nested-container environments cannot mount
# rofiles-fuse.  Disabling it is slower but portable.  Use the invoking
# user's Flatpak installation by default.  force-clean resets the app
# directory for repeat builds while preserving the state directory cache.
FLATPAK_BUILDER_OPTIONS ?= --disable-rofiles-fuse --force-clean --user

FLATPAK_OUTPUT_DIR ?= $(OUT)/UNIX/flatpak
FLATPAK_OUTPUT_DIR := $(abspath $(FLATPAK_OUTPUT_DIR))
FLATPAK_WORK_DIR ?= $(FLATPAK_OUTPUT_DIR)/work
FLATPAK_BUILD_DIR = $(FLATPAK_WORK_DIR)/build
FLATPAK_STATE_DIR = $(FLATPAK_WORK_DIR)/state
FLATPAK_REPO_DIR = $(FLATPAK_OUTPUT_DIR)/repo
FLATPAK_MANIFEST = $(FLATPAK_OUTPUT_DIR)/$(FLATPAK_APP_ID).yml
FLATPAK_BUNDLE = $(FLATPAK_OUTPUT_DIR)/$(FLATPAK_APP_ID).flatpak
FLATPAK_SOURCE_ROOT ?= $(abspath $(topdir))
FLATPAK_SOURCE_ROOT_SED = $(subst |,\|,$(subst &,\&,$(subst \,\\,$(FLATPAK_SOURCE_ROOT))))
FLATPAK_HIDDEN_SOURCE_FILES = $(shell find "$(FLATPAK_SOURCE_ROOT)" -mindepth 1 -maxdepth 1 -name '.*' -exec basename {} \; | LC_ALL=C sort | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/"/' | paste -sd, - | sed 's/^/, /')
FLATPAK_HIDDEN_SOURCE_FILES_SED = $(subst |,\|,$(subst &,\&,$(subst \,\\,$(FLATPAK_HIDDEN_SOURCE_FILES))))

FLATPAK_TEMPLATES = \
	$(topdir)/flatpak/org.xcsoar.XCSoar.yml.in \
	$(topdir)/flatpak/org.xcsoar.XCSoar.desktop.in \
	$(topdir)/flatpak/org.xcsoar.XCSoar.metainfo.xml.in

$(FLATPAK_MANIFEST): FORCE $(FLATPAK_TEMPLATES) | $(FLATPAK_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  FLATPAK  manifest $(FLATPAK_APP_ID)"
	$(Q)sed -e 's/@FLATPAK_APP_ID@/$(FLATPAK_APP_ID)/g' \
		-e 's|@FLATPAK_SOURCE_ROOT@|$(FLATPAK_SOURCE_ROOT_SED)|g' \
		-e 's|@FLATPAK_HIDDEN_SOURCE_FILES@|$(FLATPAK_HIDDEN_SOURCE_FILES_SED)|g' \
		$(topdir)/flatpak/org.xcsoar.XCSoar.yml.in > $@

.PHONY: flatpak flatpak-manifest flatpak-clean flatpak-install \
	flatpak-install-metadata FORCE

FORCE:

flatpak-manifest: $(FLATPAK_MANIFEST)

flatpak: $(FLATPAK_MANIFEST)
	@$(NQ)echo "  FLATPAK  $(FLATPAK_BUNDLE)"
	# Keep the app build and state directories on the local user filesystem,
	# where Flatpak can remove files and retain its incremental-build cache.
	$(Q)mkdir -p $(FLATPAK_WORK_DIR)
	$(Q)cd $(FLATPAK_OUTPUT_DIR) && $(FLATPAK_BUILDER) $(FLATPAK_BUILDER_OPTIONS) \
		--install-deps-from=$(FLATPAK_REMOTE) \
		--repo=$(FLATPAK_REPO_DIR) \
		--state-dir=$(FLATPAK_STATE_DIR) \
		$(FLATPAK_BUILD_DIR) $(FLATPAK_MANIFEST)
	$(Q)$(FLATPAK) build-bundle $(FLATPAK_REPO_DIR) $(FLATPAK_BUNDLE) \
		$(FLATPAK_APP_ID) --runtime-repo=$(FLATPAK_RUNTIME_REPO)

# This target is run by flatpak-builder inside the Flatpak SDK.  Keep all
# XCSoar source preparation and installation in the existing Make system;
# the manifest only describes the sandbox and calls this target.
flatpak-install: $(BOOST_UNTAR_STAMP) \
	$(TARGET_OUTPUT_DIR)/include/MakeResource.hpp \
	$(TARGET_OUTPUT_DIR)/include/ResourceLookup_entries.cpp
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
