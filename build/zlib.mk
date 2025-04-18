ifneq ($(ZLIB_PREFIX),)

# a zlib prefix was explicitly specified

ZLIB_LDADD =
ZLIB_LDLIBS = -L$(ZLIB_PREFIX)/lib -lz

ZLIB_CPPFLAGS = -isystem$(ZLIB_PREFIX)/include

else ifeq ($(TARGET),ANDROID)

# use the native zlib on ANDROID, see
# android-ndk-*/docs/STABLE-APIS.html

ZLIB_CPPFLAGS =
ZLIB_LDADD =
ZLIB_LDLIBS = -lz

else

ZLIB_LDADD =
ZLIB_LDLIBS = -lz

ZLIB_CPPFLAGS =

endif

ZLIB_CPPFLAGS += -DZLIB_CONST
