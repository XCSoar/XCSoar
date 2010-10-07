ZZIPSRC	= $(SRC)/zzip
ZZIP = \
	$(ZZIPSRC)/err.c 		$(ZZIPSRC)/fetch.c \
	$(ZZIPSRC)/file.c 		\
	$(ZZIPSRC)/info.c \
	$(ZZIPSRC)/plugin.c \
	$(ZZIPSRC)/zip.c \
	$(ZZIPSRC)/stat.c

ZZIP_LIBS = $(TARGET_OUTPUT_DIR)/zzip.a

$(ZZIP_LIBS): INCLUDES += $(ZLIB_INCLUDES)
$(ZZIP_LIBS): $(call SRC_TO_OBJ,$(ZZIP))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^

ZZIP_LIBS += $(ZLIB_LIBS)
