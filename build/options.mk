LLVM ?= n
THIN_LTO ?= n
CLANG ?= $(call bool_or,$(LLVM),$(THIN_LTO))
LTO ?= $(THIN_LTO)
IWYU ?= n
FUZZER ?= n

# shall we paint with some eye candy?
EYE_CANDY ?= $(call bool_not,$(TARGET_IS_KOBO))
ifeq ($(EYE_CANDY),y)
  TARGET_CPPFLAGS += -DEYE_CANDY
  WINDRESFLAGS += -DEYE_CANDY
endif

ifeq ($(DEBUG)$(HAVE_WIN32)$(TARGET_IS_DARWIN),nnn)
  ICF ?= y
else
  ICF ?= n
endif

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
TESTING = n

ifeq ($(TESTING),y)
  TARGET_CPPFLAGS += -DXCSOAR_TESTING
endif
