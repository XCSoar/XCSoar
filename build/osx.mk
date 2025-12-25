ifeq ($(TARGET_IS_OSX),y)

TARGET_LDLIBS += -framework AppKit -framework CoreServices

# WORKAROUND: Apple's "AssertMacros.h" header (at least in SDK 10.12) declares a
# check() method, which conflicts with our code (and boost).
# Defining __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0 avoids this
# problem.
TARGET_CPPFLAGS += -D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0

# App bundle directories
APP_BUNDLE_DIR = $(TARGET_OUTPUT_DIR)/app
DMG_TMPDIR = $(TARGET_OUTPUT_DIR)/dmg

# App Store version (must be X.Y.Z format)
MACOS_PATCH_VERSION ?= 0
MACOS_APP_VERSION ?= $(shell cat VERSION.txt).$(MACOS_PATCH_VERSION)
MACOS_APP_BUILD_NUMBER ?= 1

# App name and package names are always the same
APP_NAME = XCSoar.app
DMG_NAME = XCSoar.dmg
PKG_NAME = XCSoar.pkg
OSX_BUNDLE_ID ?= org.xcsoar.XCSoar

# Testing version uses red icon and "Testing" label
ifeq ($(TESTING),y)
APP_NAME = XCSoar-testing.app
DMG_NAME = XCSoar-testing.dmg
PKG_NAME = XCSoar-testing.pkg
OSX_APP_LABEL = XCSoar Testing
OSX_BUNDLE_ID = org.xcsoar.XCSoar-Testing
OSX_LOGO = $(DATA)/graphics/logo_red_1024.icns
else
APP_NAME = XCSoar.app
DMG_NAME = XCSoar.dmg
PKG_NAME = XCSoar.pkg
OSX_APP_LABEL = XCSoar
OSX_BUNDLE_ID = org.xcsoar.XCSoar
OSX_LOGO = $(DATA)/graphics/logo_1024.icns
endif

APP_BUNDLE = $(APP_BUNDLE_DIR)/$(APP_NAME)
APP_CONTENTS = $(APP_BUNDLE)/Contents
APP_MACOS = $(APP_CONTENTS)/MacOS
APP_RESOURCES = $(APP_CONTENTS)/Resources
APP_FRAMEWORKS = $(APP_CONTENTS)/Frameworks

# Build the .app bundle
$(APP_BUNDLE): $(TARGET_BIN_DIR)/xcsoar Data/OSX/Info.plist.in.xml $(OSX_LOGO) $(ANGLE_FIX_STAMP)
	@$(NQ)echo "  APP     $@"
	$(Q)[ ! -d $(APP_BUNDLE) ] || (chmod -R u+w $(APP_BUNDLE) && xattr -cr $(APP_BUNDLE))
	$(Q)rm -rf $(APP_BUNDLE)
	$(Q)$(MKDIR) -p $(APP_MACOS)
	$(Q)$(MKDIR) -p $(APP_RESOURCES)
	$(Q)$(MKDIR) -p $(APP_FRAMEWORKS)
	@$(NQ)echo "  PLIST   $(APP_CONTENTS)/Info.plist"
	$(Q)sed -e "s,VERSION_PLACEHOLDER,$(FULL_VERSION)," \
	    -e 's/OSX_APP_LABEL_PLACEHOLDER/$(OSX_APP_LABEL)/g' \
	    -e 's/OSX_BUNDLE_ID_PLACEHOLDER/$(OSX_BUNDLE_ID)/g' \
	    < Data/OSX/Info.plist.in.xml > $(APP_CONTENTS)/Info.plist
	@$(NQ)echo "  COPY    xcsoar binary"
	$(Q)cp $(TARGET_BIN_DIR)/xcsoar $(APP_MACOS)/
	@$(NQ)echo "  COPY    icon"
	$(Q)cp $(OSX_LOGO) $(APP_RESOURCES)/logo_1024.icns
ifeq ($(USE_ANGLE),y)
	@$(NQ)echo "  BUNDLE  ANGLE libraries"
	$(Q)cp -a $(ANGLE_PREFIX)/lib/libEGL.dylib $(APP_FRAMEWORKS)/
	$(Q)cp -a $(ANGLE_PREFIX)/lib/libGLESv2.dylib $(APP_FRAMEWORKS)/
	$(Q)chmod u+w $(APP_FRAMEWORKS)/libEGL.dylib $(APP_FRAMEWORKS)/libGLESv2.dylib
	$(Q)install_name_tool -id "@rpath/libEGL.dylib" $(APP_FRAMEWORKS)/libEGL.dylib
	$(Q)install_name_tool -id "@rpath/libGLESv2.dylib" $(APP_FRAMEWORKS)/libGLESv2.dylib
	$(Q)if ! install_name_tool -change "$$(otool -L $(APP_FRAMEWORKS)/libEGL.dylib | grep libGLESv2 | awk '{print $$1}')" "@rpath/libGLESv2.dylib" $(APP_FRAMEWORKS)/libEGL.dylib 2>/dev/null; then \
		echo "  WARN    Failed to update libEGL.dylib dependency"; \
	fi
	$(Q)if ! install_name_tool -change "./libEGL.dylib" "@rpath/libEGL.dylib" $(APP_MACOS)/xcsoar 2>/dev/null; then \
		echo "  WARN    Failed to update xcsoar libEGL.dylib reference"; \
	fi
	$(Q)if ! install_name_tool -change "./libGLESv2.dylib" "@rpath/libGLESv2.dylib" $(APP_MACOS)/xcsoar 2>/dev/null; then \
		echo "  WARN    Failed to update xcsoar libGLESv2.dylib reference"; \
	fi
endif
# Bundle Homebrew libraries and fix all @rpath references
	@$(NQ)echo "  BUNDLE  Homebrew libraries"
	$(Q)if ! install_name_tool -add_rpath "@executable_path/../Frameworks" $(APP_MACOS)/xcsoar 2>/dev/null; then \
		echo "  WARN    Failed to add rpath to xcsoar"; \
	fi
	$(Q)ANGLE_PATTERN="$(if $(ANGLE_PREFIX),|$(ANGLE_PREFIX),)"; \
	for lib in $$(otool -L $(APP_MACOS)/xcsoar | \
		grep -E "/opt/homebrew|/usr/local$$ANGLE_PATTERN" | \
		awk '{print $$1}'); do \
		[ -f "$$lib" ] || continue; \
		base=$$(basename "$$lib"); \
		[ -f "$(APP_FRAMEWORKS)/$$base" ] && continue; \
		if ! cp "$$lib" $(APP_FRAMEWORKS)/; then \
			echo "  WARN    Failed to copy $$lib"; \
			continue; \
		fi; \
		if ! chmod u+w $(APP_FRAMEWORKS)/$$base; then \
			echo "  WARN    Failed to chmod $(APP_FRAMEWORKS)/$$base"; \
		fi; \
		if ! install_name_tool -id "@rpath/$$base" $(APP_FRAMEWORKS)/$$base; then \
			echo "  WARN    Failed to set install_name for $$base"; \
		fi; \
		if ! install_name_tool -change "$$lib" "@rpath/$$base" $(APP_MACOS)/xcsoar; then \
			echo "  WARN    Failed to update xcsoar reference for $$base"; \
		fi; \
	done
# Recursively bundle dependencies of bundled libraries
	@$(NQ)echo "  BUNDLE  Transitive dependencies"
	$(Q)processed=""; \
	while true; do \
		new_libs=""; \
		for bundled in $(APP_FRAMEWORKS)/*.dylib; do \
			[ -f "$$bundled" ] || continue; \
			for lib in $$(otool -L "$$bundled" | grep -E '/opt/homebrew|/usr/local' | awk '{print $$1}'); do \
				[ -f "$$lib" ] || continue; \
				base=$$(basename "$$lib"); \
				echo "$$processed" | grep -q " $$base " && continue; \
				[ -f "$(APP_FRAMEWORKS)/$$base" ] && continue; \
				if ! cp "$$lib" $(APP_FRAMEWORKS)/; then \
					echo "  WARN    Failed to copy $$lib"; \
					continue; \
				fi; \
				if ! chmod u+w $(APP_FRAMEWORKS)/$$base; then \
					echo "  WARN    Failed to chmod $(APP_FRAMEWORKS)/$$base"; \
				fi; \
				if ! install_name_tool -id "@rpath/$$base" $(APP_FRAMEWORKS)/$$base; then \
					echo "  WARN    Failed to set install_name for $$base"; \
				fi; \
				if ! install_name_tool -change "$$lib" "@rpath/$$base" "$$bundled"; then \
					echo "  WARN    Failed to update $$bundled reference for $$base"; \
				fi; \
				new_libs="$$new_libs $$base "; \
			done; \
		done; \
		[ -z "$$new_libs" ] && break; \
		processed="$$processed$$new_libs"; \
	done
# Fix references between bundled libraries
	$(Q)for bundled in $(APP_FRAMEWORKS)/*.dylib; do \
		[ -f "$$bundled" ] || continue; \
		for lib in $$(otool -L "$$bundled" | grep -E '/opt/homebrew|/usr/local' | awk '{print $$1}'); do \
			base=$$(basename "$$lib"); \
			if [ -f "$(APP_FRAMEWORKS)/$$base" ]; then \
				if ! install_name_tool -change "$$lib" "@rpath/$$base" "$$bundled" 2>/dev/null; then \
					echo "  WARN    Failed to update $$bundled reference for $$base"; \
				fi; \
			fi; \
		done; \
	done
# Ad-hoc sign to fix invalidated signatures (for local development)
	@$(NQ)echo "  SIGN    Ad-hoc signing for local development"
	$(Q)codesign --force --deep --sign - $(APP_BUNDLE) 2>/dev/null || true

# Build the .dmg disk image
$(TARGET_OUTPUT_DIR)/$(DMG_NAME): $(APP_BUNDLE)
	@$(NQ)echo "  DMG     $@"
	$(Q)rm -rf $(DMG_TMPDIR)
	$(Q)$(MKDIR) -p $(DMG_TMPDIR)
	$(Q)cp -a $(APP_BUNDLE) $(DMG_TMPDIR)/
	$(Q)ln -s /Applications $(DMG_TMPDIR)/Applications
	$(Q)rm -f $@
	$(Q)hdiutil create -volname "$(OSX_APP_LABEL)" -srcfolder $(DMG_TMPDIR) -ov -format UDZO $@

# Build the .pkg installer
$(TARGET_OUTPUT_DIR)/$(PKG_NAME): $(APP_BUNDLE)
	@$(NQ)echo "  PKG     $@"
	$(Q)rm -f $@
	$(Q)pkgbuild --root $(APP_BUNDLE_DIR) \
	    --identifier $(OSX_BUNDLE_ID) \
	    --version $(FULL_VERSION) \
	    --install-location /Applications \
	    $@

app: $(APP_BUNDLE)

# Sign the app bundle (optional, run with: make app-sign MACOS_CODE_SIGN_ID="...")
app-sign: $(APP_BUNDLE)
ifndef MACOS_CODE_SIGN_ID
	$(error MACOS_CODE_SIGN_ID is required. Usage: make app-sign MACOS_CODE_SIGN_ID="Developer ID Application: ...")
endif
	@$(NQ)echo "  SIGN    $(APP_BUNDLE)"
	$(Q)codesign --deep --force --options runtime --timestamp -s "$(MACOS_CODE_SIGN_ID)" $(APP_BUNDLE)

dmg: $(TARGET_OUTPUT_DIR)/$(DMG_NAME)

pkg: $(TARGET_OUTPUT_DIR)/$(PKG_NAME)

endif
