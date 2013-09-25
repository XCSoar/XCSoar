%/dirstamp:
	@$(MKDIR) -p $(subst C:, ,$(abspath $(@D)))
	@touch $(abspath $@)

noop=