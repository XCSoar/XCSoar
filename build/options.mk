LLVM ?= n
THIN_LTO ?= n
CLANG ?= $(THIN_LTO)
LTO ?= $(THIN_LTO)
IWYU ?= n
FUZZER ?= n

# Build the fuzzers with libFuzzer? (i.e. -fsanitize=fuzzer) - (for
# Honggfuzz, this should be disabled)
LIBFUZZER ?= n

# Prefer packages from Homebrew?
USE_HOMEBREW ?= n

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

# Enable gcc/clang sanitizers?  Either "n" to disable, "y" to enable
# default sanitizers or a comma-separated list of sanitizers
# (e.g. "address,undefined").
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

# Default Android package flavor:
# - FOSS is the default for regular Android builds
# - PLAY/TESTING builds keep their own package IDs without requiring FOSS=n
ifneq ($(filter ANDROID%,$(TARGET)),)
  ifeq ($(PLAY),y)
    FOSS ?= n
  else ifeq ($(TESTING),y)
    FOSS ?= n
  else
    FOSS ?= y
  endif
endif

# Set XCSOAR_TESTING for non-Android builds (Android builds set it based on package name)
ifeq ($(TESTING),y)
  ifneq ($(TARGET_IS_ANDROID),y)
    TARGET_CPPFLAGS += -DXCSOAR_TESTING
  endif
endif
