LIBPNG ?= n

ifeq ($(LIBPNG),y)

$(eval $(call pkg-config-library,LIBPNG,libpng))

LIBPNG_LDADD += $(ZLIB_LDADD)
LIBPNG_LDLIBS += $(ZLIB_LDLIBS)

ifneq ($(CLANG),y)
ifneq ($(filter 4.8%,$(CXX_VERSION)),)
# detected gcc 4.8

# this option disables a C++11 warning/error in libpng due to a missing space
# in a debug macro. unfortunately a GCC bug is producing the warning even
# though the code should be disabled by the preprocessor.
# (see http://gcc.gnu.org/bugzilla//show_bug.cgi?id=58155)

LIBPNG_CPPFLAGS += -Wno-error=literal-suffix
endif
endif

endif
