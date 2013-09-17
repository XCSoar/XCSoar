CABWIZ = wine 'c:\cabwiz\cabwiz.exe'

$(TARGET_BIN_DIR)/XCSoar.inf: build/cab.inf $(TARGET_BIN_DIR)/dirstamp
	$(Q)cp $< $@

$(addprefix $(TARGET_BIN_DIR)/,$(THIRDPARTY_DLLS)): $(TARGET_BIN_DIR)/%.dll: $(THIRDPARTY_DLL_DIR)/%.dll
	$(Q)cp $< $@

$(TARGET_BIN_DIR)/XCSoar.$(PCPU).CAB: $(TARGET_BIN_DIR)/XCSoar.inf $(XCSOAR_BIN) \
	$(XCSOARSETUP_DLL) $(XCSOARLAUNCH_DLL) \
	$(addprefix $(TARGET_BIN_DIR)/,$(THIRDPARTY_DLLS))
	@$(NQ)echo "  CAB     $@"
	$(Q)cd $(TARGET_BIN_DIR) && $(CABWIZ) XCSoar.inf /cpu $(PCPU)

$(TARGET_BIN_DIR)/XCSoar-$(TARGET).cab: $(TARGET_BIN_DIR)/XCSoar.$(PCPU).CAB
	$(Q)mv $< $@

cab: $(TARGET_BIN_DIR)/XCSoar-$(TARGET).cab
