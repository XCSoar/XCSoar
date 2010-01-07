%/dirstamp:
	@mkdir -p $(abspath $(@D))
	@touch $(abspath $@)
