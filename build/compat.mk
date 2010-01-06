COMPATSRC:=$(SRC)/wcecompat
COMPAT	:=\
	$(COMPATSRC)/ts_string.cpp

ifeq ($(HAVE_POSIX)$(CONFIG_PC),nn)
COMPAT += $(COMPATSRC)/errno.cpp
endif

COMPAT_LIBS = $(TARGET_OUTPUT_DIR)/compat-$(TARGET).a

$(COMPAT_LIBS): $(call SRC_TO_OBJ,$(COMPAT))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
