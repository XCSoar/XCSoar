ifeq ($(TARGET),UNIX)

# use the native zlib on UNIX

ifeq ($(shell uname -s),Darwin)
ZLIB_LDADD = /opt/local/lib/libz.a
ZLIB_LDFLAGS =
else
ZLIB_LDADD =
ZLIB_LDFLAGS = -lz
endif

ZLIB_INCLUDES =

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

ZLIB_LDADD = $(TARGET_OUTPUT_DIR)/zlib.a
ZLIB_LDFLAGS =
ZLIB_INCLUDES = -I$(ZLIB_SRC_DIR)

$(ZLIB_LDADD): CPPFLAGS += -DNO_VIZ -DHAVE_UNISTD_H
$(ZLIB_LDADD): $(call SRC_TO_OBJ,$(ZLIB_SOURCES))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
