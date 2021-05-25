PKG_CONFIG = pkg-config

ifeq ($(USE_THIRDPARTY_LIBS),y)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(THIRDPARTY_LIBS_ROOT)/lib/pkgconfig $(PKG_CONFIG) --static
endif

ifeq ($(TARGET_IS_DARWIN),y)
  ifeq ($(DARWIN_LIBS),)
    PKG_CONFIG := $(PKG_CONFIG) --static
  else
    PKG_CONFIG := PKG_CONFIG_LIBDIR=$(DARWIN_LIBS)/lib/pkgconfig $(PKG_CONFIG) --static --define-variable=prefix=$(DARWIN_LIBS)
  endif
endif

ifeq ($(HOST_IS_WIN32)$(HAVE_WIN32)$(HAVE_CE),nyn)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=/usr/local/i686-w64-mingw32/lib/pkgconfig $(PKG_CONFIG)
endif

ifeq ($(HOST_IS_PI)$(TARGET_IS_PI),ny)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(PI)/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --define-variable=prefix=$(PI)/usr
endif

ifeq ($(HOST_IS_ARM)$(TARGET_HAS_MALI),ny)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(CUBIE)/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --define-variable=prefix=$(CUBIE)/usr
endif

call-pkg-config = $(shell $(PKG_CONFIG) --$(2) $(1) || echo ERROR)

define assign-check-error
$(1) = $$($(2))$$(if $$(filter ERROR,$$($(2))),$$(error $(3)))
endef

ifeq ($(TARGET_IS_KOBO),y)
# No -isystem on the Kobo because it may break our Musl sysroot
pkg-config-cppflags-filter = $(1)
else
pkg-config-cppflags-filter = $(patsubst -I%,-isystem %,$(1))
endif

pkg-config-ldlibs-filter = $(1)

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

$(1)_CPPFLAGS_RAW_GEN = $$(call pkg-config-cppflags-filter,$$(call call-pkg-config,$(2),cflags))
$(1)_LDLIBS_RAW_GEN = $$(call pkg-config-ldlibs-filter,$$(call call-pkg-config,$(2),libs))
$(1)_MODVERSION_RAW_GEN = $$(call call-pkg-config,$(2),modversion)

$$(foreach i,CPPFLAGS LDLIBS MODVERSION,$$(call DEF_THUNK,$(1)_$$(i)_RAW))
$$(foreach i,CPPFLAGS LDLIBS MODVERSION,$$(eval $$(call assign-check-error,$(1)_$$(i),$(1)_$$(i)_RAW,library not found: $(2))))

endef
