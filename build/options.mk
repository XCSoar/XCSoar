FIXED ?= $(call bool_or,$(DEBUG),$(call bool_not,$(HAVE_FPU)))
RADIANS ?= n
GREEN_MENU ?= y
PROFILE_NO_FILE ?= n

ifeq ($(FIXED),y)
TARGET_CPPFLAGS += -DFIXED_MATH
endif

ifeq ($(RADIANS),y)
TARGET_CPPFLAGS += -DRADIANS
endif

# shall menu buttons be painted with green background?
ifeq ($(GREEN_MENU),y)
TARGET_CPPFLAGS += -DGREEN_MENU
endif

# shall the profile be loaded from file into the registry?
ifeq ($(PROFILE_NO_FILE),y)
TARGET_CPPFLAGS += -DPROFILE_NO_FILE
endif

TARGET_CPPFLAGS += -DDISABLEAUDIOVARIO
