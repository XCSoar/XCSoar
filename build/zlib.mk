ifneq ($(ZLIB_PREFIX),)

# a zlib prefix was explicitly specified

ZLIB_LDADD =
ZLIB_LDLIBS = -L$(ZLIB_PREFIX)/lib -lz

ZLIB_CPPFLAGS = -isystem$(ZLIB_PREFIX)/include

else ifeq ($(TARGET),UNIX)

# use the native zlib on UNIX

ZLIB_LDADD =
ZLIB_LDLIBS = -lz

ZLIB_CPPFLAGS =

else ifeq ($(TARGET),ANDROID)

# use the native zlib on ANDROID, see
# android-ndk-*/docs/STABLE-APIS.html

ZLIB_CPPFLAGS =
ZLIB_LDADD =
ZLIB_LDLIBS = -lz

else ifeq ($(TARGET),PC)

ZLIB_LDADD =
ZLIB_LDLIBS = -lz

ZLIB_CPPFLAGS =

else

ZLIB_SRC_DIR = $(SRC)/zlib
ZLIB_SOURCES = \
	$(ZLIB_SRC_DIR)/adler32.c \
	$(ZLIB_SRC_DIR)/crc32.c \
	$(ZLIB_SRC_DIR)/inffast.c \
	$(ZLIB_SRC_DIR)/inflate.c \
	$(ZLIB_SRC_DIR)/inftrees.c \
	$(ZLIB_SRC_DIR)/zutil.c

ZLIB_LDLIBS =

ZLIB_CPPFLAGS_INTERNAL = -DHAVE_UNISTD_H
ZLIB_CPPFLAGS = -I$(ZLIB_SRC_DIR)

$(eval $(call link-library,zlib,ZLIB))

endif

ZLIB_CPPFLAGS += -DZLIB_CONST
