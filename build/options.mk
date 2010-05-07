FIXED ?= $(call bool_or,$(DEBUG),$(call bool_not,$(HAVE_FPU)))

ifeq ($(FIXED),y)
TARGET_CPPFLAGS += -DFIXED_MATH
endif

# shall menu buttons be painted with green background?
ifneq ($(GREEN_MENU),n)
TARGET_CPPFLAGS += -DGREEN_MENU
endif

# shall the profile be loaded from file into the registry?
ifeq ($(PROFILE_NO_FILE),y)
TARGET_CPPFLAGS += -DPROFILE_NO_FILE
endif
