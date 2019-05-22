# This file provides "make" rules for the C++ standard library.

ifneq ($(LIBCXX),)
  # using libc++

  include $(topdir)/build/libcxx.mk
  LIBSTDCXX_CXXFLAGS = $(LIBCXX_CXXFLAGS)
  LIBSTDCXX_CPPFLAGS = $(LIBCXX_CPPFLAGS)
  LIBSTDCXX_LDADD = $(LIBCXX_LDADD)
  LIBSTDCXX_LDFLAGS = $(LIBCXX_LDFLAGS)
else
  # using GNU libstdc++

  ifeq ($(DEBUG_GLIBCXX),y)
    LIBSTDCXX_CPPFLAGS += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
  endif

  ifeq ($(CLANG)$(TARGET_IS_KOBO),yn)
    # workaround to disable libstdc++'s use of "__float128" (since
    # version 4.7) because clang doesn't know that type
    LIBSTDCXX_CPPFLAGS += -D__STRICT_ANSI__ -D_GNU_SOURCE
  endif
endif

# Add the C++ standard library to every library and every program
TARGET_CXXFLAGS += $(LIBSTDCXX_CXXFLAGS)
TARGET_CPPFLAGS += $(LIBSTDCXX_CPPFLAGS)
TARGET_LDADD := $(LIBSTDCXX_LDADD) $(TARGET_LDADD)
TARGET_LDFLAGS += $(LIBSTDCXX_LDFLAGS)
