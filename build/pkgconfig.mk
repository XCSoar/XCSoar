PKG_CONFIG = pkg-config

ifeq ($(USE_THIRDPARTY_LIBS),y)
  PKG_CONFIG := PKG_CONFIG_LIBDIR=$(THIRDPARTY_LIBS_ROOT)/lib/pkgconfig $(PKG_CONFIG) --static
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

pkg-config-cppflags-filter = $(patsubst -I%,-isystem %,$(1))
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
