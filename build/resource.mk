ifeq ($(HAVE_WIN32),y)

RESOURCE_TEXT = src/XCSoar.rc
RESOURCE_BINARY = $(TARGET_OUTPUT_DIR)/$(notdir $(RESOURCE_TEXT:.rc=.rsc))

$(RESOURCE_BINARY): $(RESOURCE_TEXT) $(wildcard Data/Dialogs/*.xml)
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

else

# no resources on UNIX
RESOURCE_BINARY =

endif
