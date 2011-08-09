RADIANS ?= y

FIXED ?= $(call bool_not,$(HAVE_FPU))
ifeq ($(FIXED),y)
TARGET_CPPFLAGS += -DFIXED_MATH
endif

ifeq ($(RADIANS),y)
TARGET_CPPFLAGS += -DRADIANS
endif

# shall we paint with some eye candy?
ifneq ($(EYE_CANDY),n)
TARGET_CPPFLAGS += -DEYE_CANDY
WINDRESFLAGS += -DEYE_CANDY
endif

ifeq ($(TARGET),ANDROID)
ifneq ($(IOIOLIB_DIR),)
TARGET_CPPFLAGS += -DIOIOLIB
endif
endif

# show map renderer times?
STOP_WATCH ?= n
ifeq ($(STOP_WATCH),y)
TARGET_CPPFLAGS += -DSTOP_WATCH
endif
