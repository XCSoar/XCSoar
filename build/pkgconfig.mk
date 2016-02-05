PKG_CONFIG = pkg-config

ifeq ($(USE_THIRDARTY_LIBS),y)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(THIRDARTY_LIBS_ROOT)/lib/pkgconfig $(PKG_CONFIG) --static
endif

ifeq ($(HOST_IS_WIN32)$(HAVE_WIN32),ny)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=/usr/local/i686-w64-mingw32/lib/pkgconfig $(PKG_CONFIG)
endif

ifeq ($(HOST_IS_PI)$(TARGET_IS_PI),ny)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(PI)/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --define-variable=prefix=$(PI)/usr
endif

ifeq ($(HOST_IS_ARM)$(TARGET_HAS_MALI),ny)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(CUBIE)/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --define-variable=prefix=$(CUBIE)/usr
endif

call-pkg-config = $(shell $(PKG_CONFIG) --$(2) $(1))

# Generates a pkg-config lookup for a library.
#
# Example: $(eval $(call pkg-config-library,CURL,libcurl >= 2.21))
#
# Arguments: PREFIX, SPEC
#
# PREFIX is a prefix for variables that will hold the results.  This
# function will append "_CPPFLAGS" (pkg-config --cflags) and "_LDLIBS"
# (pkg-config --libs).
#
# SPEC is the pkg-config package specification.
#
define pkg-config-library

ifneq ($$(shell $$(PKG_CONFIG) --exists $(2) && echo ok),ok)
$$(error library not found: $(2))
endif

$(1)_CPPFLAGS := $$(patsubst -I%,-isystem %,$$(call call-pkg-config,$(2),cflags))
$(1)_LDLIBS := $$(call call-pkg-config,$(2),libs)
$(1)_MODVERSION := $$(call call-pkg-config,$(2),modversion)

ifeq ($$(TARGET)$$(ARMV7),ANDROIDy)
# Android-ARMv7 requires "-lm_hard" instead of "-lm"; some libraries
# such as libtiff however hard-code "-lm" in their pkg-config file,
# which causes serious math breakage; therefore, filter out all "-lm"
# flags.
$(1)_LDLIBS := $$(filter-out -lm,$$($(1)_LDLIBS))
endif

endef
