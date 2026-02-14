# NSIS installer build script for Windows targets
# Creates installer packages for XCSoar Windows builds

ifeq ($(HAVE_WIN32),y)

# Only build installers for WIN64OPENGL target (can be extended later)
ifeq ($(TARGET_FLAVOR),WIN64OPENGL)

# Check for VERSION.txt
ifeq ($(wildcard VERSION.txt),)
$(error VERSION.txt is missing)
endif

# Read version from VERSION.txt
XCSOAR_VERSION := $(shell cat VERSION.txt)

# Product name (from version.mk)
PRODUCT_NAME ?= XCSoar

# Testing version support
ifeq ($(TESTING),y)
INSTALLER_LABEL = $(PRODUCT_NAME) Testing
else
INSTALLER_LABEL = $(PRODUCT_NAME)
endif

# NSIS compiler (makensis)
# On Linux cross-compile, we need makensis from nsis package
# On Windows, use the native makensis
MAKENSIS ?= makensis

# NSIS script location
NSIS_SCRIPT = $(topdir)/windows/xcsoar.nsi

# Installer output file (in bin directory)
INSTALLER_NAME = $(PRODUCT_NAME)-$(XCSOAR_VERSION)-WIN64OPENGL-Installer.exe
INSTALLER_OUTPUT = $(TARGET_BIN_DIR)/$(INSTALLER_NAME)

# Dependencies: XCSoar.exe and ANGLE DLLs must be built first
INSTALLER_DEPS = $(TARGET_BIN_DIR)/XCSoar.exe
ifeq ($(USE_ANGLE),y)
INSTALLER_DEPS += $(TARGET_BIN_DIR)/libEGL.dll $(TARGET_BIN_DIR)/libGLESv2.dll
endif

# Build the installer
$(INSTALLER_OUTPUT): $(INSTALLER_DEPS) $(NSIS_SCRIPT) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  NSIS    $@"
	$(Q)cd $(topdir)/windows && $(MAKENSIS) \
		-DPRODUCT_NAME="$(PRODUCT_NAME)" \
		-DPRODUCT_VERSION="$(XCSOAR_VERSION)" \
		-DINSTALLER_LABEL="$(INSTALLER_LABEL)" \
		-DOUTPUT_FILE="$(abspath $@)" \
		-DBIN_DIR="$(abspath $(TARGET_BIN_DIR))" \
		xcsoar.nsi

# Phony target for easy invocation
installer: $(INSTALLER_OUTPUT)

.PHONY: installer

endif # WIN64OPENGL

endif # HAVE_WIN32
