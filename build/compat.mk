COMPATSRC:=$(SRC)/wcecompat
COMPAT	:=\
	$(COMPATSRC)/ts_string.cpp

ifeq ($(HAVE_POSIX)$(CONFIG_PC),nn)
COMPAT += $(COMPATSRC)/errno.cpp
endif

$(SRC)/compat-$(TARGET).a: $(call SRC_TO_OBJ,$(COMPAT))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
