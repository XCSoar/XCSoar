RADIANS ?= y
LLVM ?= n
CLANG ?= $(LLVM)

FIXED ?= $(call bool_not,$(HAVE_FPU))
ifeq ($(FIXED),y)
TARGET_CPPFLAGS += -DFIXED_MATH
endif

ifeq ($(RADIANS),y)
TARGET_CPPFLAGS += -DRADIANS
endif

# shall we paint with some eye candy?
EYE_CANDY ?= $(call bool_not,$(TARGET_IS_KOBO))
ifeq ($(EYE_CANDY),y)
TARGET_CPPFLAGS += -DEYE_CANDY
WINDRESFLAGS += -DEYE_CANDY
endif

# show map renderer times?
STOP_WATCH ?= n
ifeq ($(STOP_WATCH),y)
TARGET_CPPFLAGS += -DSTOP_WATCH
endif

# compile without UI?
HEADLESS ?= n

ifeq ($(TARGET_IS_KOBO),y)
DITHER ?= y
else
DITHER ?= n
endif

ifeq ($(DITHER),y)
TARGET_CPPFLAGS += -DDITHER
endif

GREYSCALE ?= $(DITHER)

ifeq ($(GREYSCALE),y)
TARGET_CPPFLAGS += -DGREYSCALE
endif
