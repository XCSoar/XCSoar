# libc++, the C++ standard library implementation from the LLVM
# project.

ifeq ($(LIBCXX),y)

ifeq ($(TARGET),ANDROID)
LIBCXX_CXXFLAGS = -nostdinc++ \
	-isystem $(ANDROID_NDK)/sources/cxx-stl/llvm-libc++/include \
	-isystem $(ANDROID_NDK)/sources/android/support/include \
	-DLIBCXX
  LIBCXX_LDADD = $(ANDROID_NDK)/sources/cxx-stl/llvm-libc++/libs/$(ANDROID_ABI3)/libc++_static.a \
	$(ANDROID_NDK)/sources/cxx-stl/llvm-libc++/libs/$(ANDROID_ABI3)/libc++abi.a

  ifeq ($(ARMV7),y)
    LIBCXX_LDADD += $(ANDROID_NDK)/sources/cxx-stl/llvm-libc++/libs/$(ANDROID_ABI3)/libunwind.a
  endif

else
LIBCXX_CXXFLAGS += -stdlib=libc++
LIBCXX_LDFLAGS += -stdlib=libc++
endif

else

LIBCXX_CXXFLAGS_INTERNAL = -Wno-char-subscripts -Wno-sign-compare
LIBCXX_CPPFLAGS = -nostdinc++ -isystem $(LIBCXX)/include -DLIBCXX
LIBCXX_SOURCES = \
	$(SRC)/LibCXX.cpp \
	$(LIBCXX)/src/algorithm.cpp \
	$(LIBCXX)/src/hash.cpp \
	$(LIBCXX)/src/string.cpp

ifeq ($(TARGET),ANDROID)
LIBCXX_CPPFLAGS_INTERNAL += -D__APPLE__=0
LIBCXX_SOURCES += $(LIBCXX)/src/new.cpp
endif

$(eval $(call link-library,libcxx,LIBCXX))

endif
