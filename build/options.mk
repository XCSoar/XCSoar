RADIANS ?= y
GREEN_MENU ?= y

FIXED ?= $(call bool_not,$(HAVE_FPU))
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

# shall we paint with some eye candy?
ifneq ($(EYE_CANDY),n)
TARGET_CPPFLAGS += -DEYE_CANDY
WINDRESFLAGS += -DEYE_CANDY
endif

# show render timings on the map?
DRAW_LOAD ?= n
ifeq ($(DRAW_LOAD),y)
TARGET_CPPFLAGS += -DDRAWLOAD
endif

TARGET_CPPFLAGS += -DDISABLEAUDIOVARIO
