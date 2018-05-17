LLVM ?= n
CLANG ?= $(LLVM)
IWYU ?= n

# shall we paint with some eye candy?
EYE_CANDY ?= $(call bool_not,$(TARGET_IS_KOBO))
ifeq ($(EYE_CANDY),y)
TARGET_CPPFLAGS += -DEYE_CANDY
WINDRESFLAGS += -DEYE_CANDY
endif

ICF ?= n

# enable gcc/clang sanitizers?
SANITIZE ?= n

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

# When enabled, the Androidpackage org.xcsoar.testing is created, with
# a red Activity icon, to allow simultaneous installation of "stable"
# and "testing".
# In the stable branch, this should default to "n".
TESTING = y

ifeq ($(TESTING),y)
  TARGET_CPPFLAGS += -DTESTING
endif
