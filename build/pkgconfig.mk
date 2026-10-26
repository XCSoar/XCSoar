PKG_CONFIG := pkg-config


ifeq ($(USE_THIRDPARTY_LIBS),y)
  ifeq ($(TARGET_IS_ANDROID)$(TARGET_IS_KOBO)$(TARGET_IS_IOS),nnn)
    # Prefer packages built by build/thirdparty.py, but allow packages omitted
    # from THIRDPARTY_PACKAGES to be discovered on the native system.
    PKG_CONFIG := PKG_CONFIG_PATH=$(THIRDPARTY_LIBS_ROOT)/lib/pkgconfig $(PKG_CONFIG) --static
  else
    # Cross builds for non standard Linux systems must not fall back to host pkg-config metadata.
    # Linux Cross-build are using SYSROOT. See below.
    PKG_CONFIG := PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR=$(THIRDPARTY_LIBS_ROOT)/lib/pkgconfig $(PKG_CONFIG) --static
  endif
endif

ifeq ($(TARGET_IS_DARWIN),y)
  ifeq ($(DARWIN_LIBS),)
    PKG_CONFIG := $(PKG_CONFIG) --static
  else
    # Clear external PKG_CONFIG_PATH to prevent inadvertedly using
    # host packages instead only the ones from $(DARWIN_LIBS)
    PKG_CONFIG_PATH := 
    export PKG_CONFIG_PATH
    PKG_CONFIG := PKG_CONFIG_LIBDIR=$(DARWIN_LIBS)/lib/pkgconfig $(PKG_CONFIG) --static --define-variable=prefix=$(DARWIN_LIBS)
  endif
endif

# Redirect package detection to SYSROOT when it is defined for a standard Linux
# target.
ifneq ($(SYSROOT),)
  # when no toolchain is explicitly defined, obtain it from your compiler.
  ifeq ($(HOST_TRIPLET),)
    HOST_TRIPLET = $(shell $(CXX) -dumpmachine)
  endif
  # Clear external PKG_CONFIG_PATH to prevent inadvertedly using
  # host packages instead only the ones from $(SYSROOT)
  PKG_CONFIG_PATH := 
  export PKG_CONFIG_PATH
  CROSS_PKG_CONFIG_LIBDIR := $(SYSROOT)/usr/lib/$(HOST_TRIPLET)/pkgconfig:$(SYSROOT)/usr/share/pkgconfig
  PKG_CONFIG := PKG_CONFIG_SYSROOT_DIR=$(SYSROOT) PKG_CONFIG_LIBDIR=$(CROSS_PKG_CONFIG_LIBDIR) $(PKG_CONFIG)
endif

call-pkg-config = $(shell $(PKG_CONFIG) --$(2) $(1) || echo ERROR)
call-pkg-config-unstatic = $(shell $(subst --static,,$(PKG_CONFIG)) --$(2) $(1) || echo ERROR)

define assign-check-error
$(1) = $$($(2))$$(if $$(filter ERROR,$$($(2))),$$(error $(3)))
endef

# Like pkg-config-library, but omit --static from the configured command.
# Some native package configurations only provide valid transitive dependency
# metadata for their shared libraries.
define pkg-config-library-unstatic

$(1)_CPPFLAGS_RAW_GEN = $$(call pkg-config-cppflags-filter,$$(call call-pkg-config-unstatic,$(2),cflags))
$(1)_LDLIBS_RAW_GEN = $$(call pkg-config-ldlibs-filter,$$(call call-pkg-config-unstatic,$(2),libs))
$(1)_MODVERSION_RAW_GEN = $$(call call-pkg-config-unstatic,$(2),modversion)

$$(foreach i,CPPFLAGS LDLIBS MODVERSION,$$(call DEF_THUNK,$(1)_$$(i)_RAW))
$$(foreach i,CPPFLAGS LDLIBS MODVERSION,$$(eval $$(call assign-check-error,$(1)_$$(i),$(1)_$$(i)_RAW,library not found: $(2))))

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
# Example: $(eval $(call pkg-config-library,CURL,libcurl '>=' 2.21))
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
