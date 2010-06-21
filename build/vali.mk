# Rules for VALI-XCS.exe, the non-interactive G record validation tool

VALI_XCS = $(TARGET_BIN_DIR)/VALI-XCS$(TARGET_EXEEXT)
VALI_XCS_NS = $(TARGET_BIN_DIR)/VALI-XCS$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT)

VALI_XCS_SOURCES = \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/VALI-XCS.cpp
VALI_XCS_OBJS = $(call SRC_TO_OBJ,$(VALI_XCS_SOURCES))
VALI_XCS_LDADD = $(IO_LIBS)

ifneq ($(VALI_XCS),$(VALI_XCS_NS))
$(VALI_XCS): $(VALI_XCS_NS)
	@$(NQ)echo "  STRIP   $@"
	$(Q)$(STRIP) $< -o $@
	$(Q)$(SIZE) $@
endif

$(VALI_XCS_NS): $(VALI_XCS_OBJS) $(VALI_XCS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(SCREEN_LDLIBS) -o $@
