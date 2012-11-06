ifeq ($(TARGET),UNIX)

# use the native zlib on UNIX

ifeq ($(TARGET_IS_DARWIN),y)
ZLIB_LDADD = /opt/local/lib/libz.a
ZLIB_LDLIBS =
else
ZLIB_LDADD =
ZLIB_LDLIBS = -lz
endif

ZLIB_CPPFLAGS =

else
ifeq ($(TARGET),ANDROID)

# use the native zlib on ANDROID, see
# android-ndk-*/docs/STABLE-APIS.html

ZLIB_CPPFLAGS =
ZLIB_LDADD = $(ANDROID_TARGET_ROOT)/usr/lib/libz.so
ZLIB_LDLIBS =

else

ZLIB_SRC_DIR = $(SRC)/zlib
ZLIB_SOURCES = \
	$(ZLIB_SRC_DIR)/adler32.c \
	$(ZLIB_SRC_DIR)/compress.c \
	$(ZLIB_SRC_DIR)/crc32.c \
	$(ZLIB_SRC_DIR)/deflate.c \
	$(ZLIB_SRC_DIR)/infback.c \
	$(ZLIB_SRC_DIR)/inffast.c \
	$(ZLIB_SRC_DIR)/inflate.c \
	$(ZLIB_SRC_DIR)/inftrees.c \
	$(ZLIB_SRC_DIR)/trees.c \
	$(ZLIB_SRC_DIR)/uncompr.c \
	$(ZLIB_SRC_DIR)/zutil.c

ZLIB_LDLIBS =

ZLIB_CPPFLAGS_INTERNAL = -DNO_VIZ -DHAVE_UNISTD_H
ZLIB_CPPFLAGS = -I$(ZLIB_SRC_DIR)

$(eval $(call link-library,zlib,ZLIB))

endif
endif
