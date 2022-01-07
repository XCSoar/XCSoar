# libc++, the C++ standard library implementation from the LLVM
# project.

ifeq ($(LIBCXX),y)

ifeq ($(TARGET),ANDROID)
  LIBCXX_CXXFLAGS = \
	-DLIBCXX
  LIBCXX_LDFLAGS += -static-libstdc++

  ifeq ($(ARMV7),y)
    LIBCXX_LDFLAGS += -lunwind
  endif

else
LIBCXX_CXXFLAGS += -stdlib=libc++
LIBCXX_LDFLAGS += -stdlib=libc++
endif

endif
