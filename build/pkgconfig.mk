PKG_CONFIG = pkg-config

ifeq ($(TARGET_IS_DARWIN),y)
  ifeq ($(DARWIN_LIBS),)
    PKG_CONFIG := $(PKG_CONFIG) --static
  else
    PKG_CONFIG := PKG_CONFIG_LIBDIR=$(DARWIN_LIBS)/lib/pkgconfig $(PKG_CONFIG) --static --define-variable=prefix=$(DARWIN_LIBS)
  endif
endif

ifeq ($(HOST_IS_WIN32)$(HAVE_WIN32)$(HAVE_CE)$(call string_equals,WINE,$(TARGET)),nynn)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=/usr/local/i686-w64-mingw32/lib/pkgconfig $(PKG_CONFIG)
endif

ifeq ($(TARGET_IS_KOBO),y)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(KOBO)/lib/pkgconfig $(PKG_CONFIG)
endif

ifeq ($(HOST_IS_PI)$(TARGET_IS_PI),ny)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(PI)/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --define-variable=prefix=$(PI)/usr
endif

ifeq ($(HOST_IS_ARM)$(TARGET_HAS_MALI),ny)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(CUBIE)/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --define-variable=prefix=$(CUBIE)/usr
endif

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

$(1)_CPPFLAGS := $$(shell $$(PKG_CONFIG) --cflags $(2))
$(1)_LDLIBS := $$(shell $$(PKG_CONFIG) --libs $(2))
$(1)_MODVERSION := $$(shell $$(PKG_CONFIG) --modversion $(2))

endef
