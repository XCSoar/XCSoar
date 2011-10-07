ZZIPSRC	= $(SRC)/zzip
ZZIP = \
	$(ZZIPSRC)/fetch.c \
	$(ZZIPSRC)/file.c 		\
	$(ZZIPSRC)/plugin.c \
	$(ZZIPSRC)/zip.c \
	$(ZZIPSRC)/stat.c

ZZIP_LIBS = $(TARGET_OUTPUT_DIR)/zzip.a

$(ZZIP_LIBS): CFLAGS += -Wno-strict-aliasing
$(ZZIP_LIBS): INCLUDES += $(ZLIB_INCLUDES)
$(ZZIP_LIBS): $(call SRC_TO_OBJ,$(ZZIP))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^

ZZIP_LIBS += $(ZLIB_LDADD)
ZZIP_LDLIBS += $(ZLIB_LDFLAGS)
