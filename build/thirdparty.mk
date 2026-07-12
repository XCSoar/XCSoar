ifeq ($(TARGET_IS_KOBO),y)
  USE_THIRDPARTY_LIBS = y
else ifeq ($(TARGET),PC)
  USE_THIRDPARTY_LIBS = y
else ifeq ($(TARGET),ANDROID)
  ifeq ($(FAT_BINARY),y)
    # this is handled by android.mk
    USE_THIRDPARTY_LIBS = n
  else
    USE_THIRDPARTY_LIBS = y
  endif
else ifeq ($(TARGET_IS_IOS),y)
  USE_THIRDPARTY_LIBS = y
else ifeq ($(TARGET_IS_DARWIN),y)
  ifeq ($(TARGET_IS_IOS),n)
    # macOS desktop: build only ANGLE through the third-party pipeline by
    # default.  Omitted packages are discovered on the system.
    USE_ANGLE ?= y
    ifeq ($(USE_ANGLE),y)
      USE_THIRDPARTY_LIBS = y
      THIRDPARTY_PACKAGES ?= angle
    endif
  endif
else
  USE_THIRDPARTY_LIBS = n
endif

ifeq ($(USE_THIRDPARTY_LIBS),y)

THIRDPARTY_PACKAGES ?= auto
empty :=
space := $(empty) $(empty)
comma := ,
THIRDPARTY_PACKAGES_LIST = $(sort $(strip $(subst $(comma), ,$(THIRDPARTY_PACKAGES))))
THIRDPARTY_PACKAGES_NORMALIZED = $(subst $(space),$(comma),$(THIRDPARTY_PACKAGES_LIST))
THIRDPARTY_PACKAGES_TAG = $(if $(THIRDPARTY_PACKAGES_NORMALIZED),$(subst $(comma),-,$(THIRDPARTY_PACKAGES_NORMALIZED)),none)
THIRDPARTY_USE_ANGLE = $(if $(USE_ANGLE),$(USE_ANGLE),auto)
THIRDPARTY_CONFIG_TAG = $(THIRDPARTY_PACKAGES_TAG)-angle-$(THIRDPARTY_USE_ANGLE)

# Helper: returns y if the named package is part of the current third-party
# selection, n otherwise.  With THIRDPARTY_PACKAGES=auto, every package is
# considered selected.
THIRDPARTY_PACKAGE_SELECTED = $(if $(filter auto,$(THIRDPARTY_PACKAGES)),y,$(if $(filter $(1),$(THIRDPARTY_PACKAGES_LIST)),y,n))

# -Wl,--gc-sections breaks the (Kobo) glibc build
THIRDPARTY_LDFLAGS_FILTER_OUT = -L$(THIRDPARTY_LIBS_DIR)/% -Wl,--gc-sections

THIRDPARTY_LIBS_DIR = $(ARCH_OUTPUT_DIR)/lib/$(THIRDPARTY_CONFIG_TAG)
THIRDPARTY_LIBS_ROOT = $(THIRDPARTY_LIBS_DIR)/$(HOST_TRIPLET)
THIRDPARTY_LIBS_STAMP = $(THIRDPARTY_LIBS_DIR)/stamp

.PHONY: libs
libs: $(THIRDPARTY_LIBS_STAMP)

compile-depends += $(THIRDPARTY_LIBS_STAMP)
$(THIRDPARTY_LIBS_STAMP):
	GEOTIFF=$(GEOTIFF) THIRDPARTY_PACKAGES="$(THIRDPARTY_PACKAGES_NORMALIZED)" USE_ANGLE=$(THIRDPARTY_USE_ANGLE) ./build/thirdparty.py $(THIRDPARTY_LIBS_DIR) $(HOST_TRIPLET) $(TARGET_IS_IOS) "$(TARGET_ARCH)" "$(TARGET_CPPFLAGS)" "$(filter-out $(THIRDPARTY_LDFLAGS_FILTER_OUT),$(TARGET_LDFLAGS))" "$(WRAPPED_CC)" "$(WRAPPED_CXX)" $(AR) "$(ARFLAGS)" $(RANLIB) $(STRIP) "$(WINDRES)" $(ENABLE_SDL)
	touch $@

ifeq ($(TARGET_IS_KOBO),n)
TARGET_CPPFLAGS += -isystem $(THIRDPARTY_LIBS_ROOT)/include
TARGET_LDFLAGS += -L$(THIRDPARTY_LIBS_ROOT)/lib
endif

endif

ifeq ($(TARGET_IS_KOBO),y)
  # we build a toolchain as part of the thirdparty-library build
  BUILD_TOOLCHAIN_TARGET = $(THIRDPARTY_LIBS_STAMP)
else
  BUILD_TOOLCHAIN_TARGET =
endif

compile-depends += boost
