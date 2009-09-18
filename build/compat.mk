COMPATSRC:=$(SRC)/wcecompat
COMPAT	:=\
	$(COMPATSRC)/ts_string.cpp

ifneq ($(CONFIG_WINE),y)
COMPAT += $(COMPATSRC)/errno.cpp
endif

$(SRC)/compat-$(TARGET).a: $(patsubst %.cpp,%-$(TARGET).o,$(COMPAT:.c=-$(TARGET).o))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
