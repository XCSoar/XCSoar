%/dirstamp:
	@$(call create-directory,$(@D))
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


# Creates a new directory.
# mkdir -p on UNIX platforms.
#
# Example: $(call create-directory,foobar)
#
# Arguments: FOLDER
ifeq ($(WINHOST),y)
MKDIR ?= "c:\Program Files\GnuWin32\bin\mkdir"
create-directory = $(MKDIR) -p "$(1)"
else
MKDIR ?= mkdir
create-directory = $(MKDIR) -p $(abspath $(1))
endif
