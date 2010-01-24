FIXED ?= $(call bool_or,$(DEBUG),$(call bool_not,$(HAVE_FPU)))

ifeq ($(FIXED),y)
TARGET_CPPFLAGS += -DFIXED_MATH
endif
