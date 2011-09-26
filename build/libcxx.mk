# libc++, the C++ standard library implementation from the LLVM
# project.

LIBCXX_CXXFLAGS = -Wno-char-subscripts -Wno-sign-compare
LIBCXX_CPPFLAGS = -nostdinc++ -isystem $(LIBCXX)/include -DLIBCXX
LIBCXX_CPPFLAGS_INTERNAL = -D_WIN32=0
LIBCXX_SOURCES = \
	$(SRC)/LibCXX.cpp \
	$(LIBCXX)/src/hash.cpp \
	$(LIBCXX)/src/string.cpp

$(eval $(call link-library,libcxx,LIBCXX))
