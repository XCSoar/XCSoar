VERSION = $(strip $(shell cat $(topdir)/VERSION.txt))
FULL_VERSION = $(VERSION)

# VERSION.txt is major.minor or major.minor.patch (policy.rst / release.rst).
VERSION_WORDS := $(subst ., ,$(VERSION))
VERSION_MAJOR := $(word 1,$(VERSION_WORDS))
VERSION_MINOR := $(word 2,$(VERSION_WORDS))
VERSION_PATCH := $(or $(word 3,$(VERSION_WORDS)),0)
VERSION_BUILD ?= 0

# Always three components for stores that require X.Y.Z (iOS / macOS).
VERSION_SHORT = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# Four components for Windows PE VERSIONINFO and NSIS VIProductVersion.
VERSION_QUAD = $(VERSION_SHORT).$(VERSION_BUILD)

# Android versionName omits a trailing .0; versionCode uses
# major*10^7 + minor*10^5 + patch*10^4 + build (build 0 for tags).
ifeq ($(VERSION_PATCH),0)
ANDROID_VERSION_NAME ?= $(VERSION_MAJOR).$(VERSION_MINOR)
else
ANDROID_VERSION_NAME ?= $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
endif
ANDROID_VERSION_BUILD ?= 0
ANDROID_VERSION_CODE ?= $(shell echo $$(($(VERSION_MAJOR)*10000000+$(VERSION_MINOR)*100000+$(VERSION_PATCH)*10000+$(ANDROID_VERSION_BUILD))))

# Product name (default: XCSoar, can be overridden via PRODUCT_NAME variable)
PRODUCT_NAME ?= XCSoar

# Lowercase product name for Unix directory names (e.g., .xcsoar, /etc/xcsoar)
PRODUCT_NAME_LOWER := $(shell echo $(PRODUCT_NAME) | tr '[:upper:]' '[:lower:]')

# Product name defines for compilation (used in VERSION_CPPFLAGS below)
# NOTE: These must match the #ifndef guards in src/ProductName.hpp
PRODUCT_NAME_CPPFLAGS = -DPRODUCT_NAME=\"$(PRODUCT_NAME)\" -DPRODUCT_NAME_LC=\"$(PRODUCT_NAME_LOWER)\"

VERSION_CPPFLAGS = -DXCSOAR_VERSION=\"$(VERSION)\" $(PRODUCT_NAME_CPPFLAGS)

GIT_COMMIT_ID := $(shell git rev-parse --short --verify HEAD 2>$(NUL))
RELEASE_COMMIT_ID := $(shell git rev-parse --short --verify "v$(VERSION)^{commit}" 2>$(NUL))
# only append the commit id for unreleased builds (no release tag)
ifneq ($(GIT_COMMIT_ID),$(RELEASE_COMMIT_ID))
VERSION_CPPFLAGS += -DGIT_COMMIT_ID=\"$(GIT_COMMIT_ID)\"
FULL_VERSION := $(FULL_VERSION)~$(GIT_COMMIT_ID)
endif

# windres -D does not keep C-string quotes; stringify these tokens in
# Data/XCSoarIcon.rc.  Do not pass VERSION_CPPFLAGS to windres.
WINDRES_VERSIONFLAGS = \
	-DXCSOAR_VERSION_MAJOR=$(VERSION_MAJOR) \
	-DXCSOAR_VERSION_MINOR=$(VERSION_MINOR) \
	-DXCSOAR_VERSION_PATCH=$(VERSION_PATCH) \
	-DXCSOAR_VERSION_BUILD=$(VERSION_BUILD) \
	-DXCSOAR_VERSION=$(VERSION) \
	-DPRODUCT_NAME=$(PRODUCT_NAME) \
	-DXCSOAR_FILE_VERSION=$(FULL_VERSION) \
	-DXCSOAR_ORIGINAL_FILENAME=$(PRODUCT_NAME).exe
ifneq ($(FULL_VERSION),$(VERSION))
WINDRES_VERSIONFLAGS += -DGIT_COMMIT_ID
endif

$(call SRC_TO_OBJ,$(SRC)/Version.cpp): $(topdir)/VERSION.txt
$(call SRC_TO_OBJ,$(SRC)/Version.cpp): CPPFLAGS += $(VERSION_CPPFLAGS)
$(call SRC_TO_OBJ,$(SRC)/Apple/MacOSMainMenu.cpp): CPPFLAGS += $(VERSION_CPPFLAGS)
$(call SRC_TO_OBJ,$(SRC)/Dialogs/dlgCredits.cpp): CPPFLAGS += $(VERSION_CPPFLAGS)
