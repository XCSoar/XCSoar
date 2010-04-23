CABWIZ = wine 'c:\cabwiz\cabwiz.exe'

$(TARGET_BIN_DIR)/XCSoar.inf: build/cab.inf
	$(Q)cp $< $@

$(TARGET_BIN_DIR)/XCSoar.$(PCPU).CAB: $(TARGET_BIN_DIR)/XCSoar.inf $(OUTPUTS) $(TARGET_BIN_DIR)/XCSoarSetup.dll $(TARGET_BIN_DIR)/XCSoarLaunch.dll
	@$(NQ)echo "  CAB     $@"
	$(Q)cd $(TARGET_BIN_DIR) && $(CABWIZ) XCSoar.inf /cpu $(PCPU)

$(TARGET_BIN_DIR)/XCSoar-$(TARGET).cab: $(TARGET_BIN_DIR)/XCSoar.$(PCPU).CAB
	$(Q)mv $< $@

cab: $(TARGET_BIN_DIR)/XCSoar-$(TARGET).cab
