%/dirstamp:
	@mkdir -p $(abspath $(@D))
	@touch $(abspath $@)


# Replaces slashes by backslashes on Windows.
#
# Example: $(call slash-subst,./foo/bar)
#
# Arguments: PATH
ifeq ($(WINHOST),y)
slash-subst = $(subst /,\\,$(1))
else
slash-subst = $(1)
endif
