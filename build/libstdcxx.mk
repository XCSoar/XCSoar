# This file provides "make" rules for the C++ standard library.

ifeq ($(TARGET),ANDROID)
  LIBSTDCXX_CPPFLAGS = -isystem $(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/$(ANDROID_GCC_VERSION)/include \
	-isystem $(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/$(ANDROID_GCC_VERSION)/libs/$(ANDROID_ABI3)/include
  LIBSTDCXX_LDADD = $(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/$(ANDROID_GCC_VERSION)/libs/$(ANDROID_ABI3)/libgnustl_static.a
endif

ifneq ($(LIBCXX),)
  # using libc++

  include $(topdir)/build/libcxx.mk
  LIBSTDCXX_CPPFLAGS = $(LIBCXX_CPPFLAGS)
  LIBSTDCXX_LDADD = $(LIBCXX_LDADD)
  LIBSTDCXX_LDFLAGS = $(LIBCXX_LDFLAGS)
else
  # using GNU libstdc++

  ifeq ($(DEBUG),y)
    LIBSTDCXX_CPPFLAGS += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
  endif
endif

# Add the C++ standard library to every library and every program
TARGET_CPPFLAGS += $(LIBSTDCXX_CPPFLAGS)
TARGET_LDADD := $(LIBSTDCXX_LDADD) $(TARGET_LDADD)
