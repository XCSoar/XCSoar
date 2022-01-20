# Make library that auto-detects the build environment.

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

HOST_IS_LINUX := $(call string_equals,$(UNAME_S),Linux)
HOST_IS_DARWIN := $(call string_equals,$(UNAME_S),Darwin)
HOST_IS_CYGWIN := $(call string_equals,$(UNAME_S),Cygwin)
HOST_IS_MINGW := $(call string_contains,$(UNAME_S),MINGW)
HOST_IS_WIN32 := $(call bool_or,$(HOST_IS_CYGWIN),$(HOST_IS_MINGW))
HOST_IS_UNIX := $(call bool_not,$(HOST_IS_WIN32))

HOST_IS_X86_32 := $(call bool_or,$(call string_contains,$(UNAME_M),i386),$(call string_contains,$(UNAME_M),i686))
HOST_IS_X86_64 := $(call bool_or,$(call string_contains,$(UNAME_M),x86_64),$(call string_contains,$(UNAME_M),amd64))

HOST_IS_ARM := $(call string_contains,$(UNAME_M),armv)
HOST_IS_ARMV6 := $(call string_equals,$(UNAME_M),armv6l)
HOST_IS_ARMV7 := $(call string_equals,$(UNAME_M),armv7l)
HOST_IS_AARCH64 := $(call string_contains,$(UNAME_M),aarch64)

HOST_IS_ARM_OR_AARCH64 = $(call bool_or,$(HOST_IS_ARM),$(HOST_IS_AARCH64))

ifeq ($(HOST_IS_ARMV7),y)
HOST_HAS_NEON := $(call string_contains,$(shell grep -E ^Features /proc/cpuinfo),neon)
else
HOST_HAS_NEON := n
endif

ifeq ($(HOST_IS_LINUX)$(HOST_IS_ARM_OR_AARCH64),yy)
HOST_IS_PI := $(call string_contains,$(shell cat /sys/firmware/devicetree/base/model 2>/dev/null),Raspberry)
else
HOST_IS_PI := n
endif

ifeq ($(HOST_IS_LINUX)$(HOST_IS_ARMV7),yy)
HOST_IS_CUBIE := $(call string_contains,$(shell cat /sys/firmware/devicetree/base/model 2>/dev/null),Cubietech)
else
HOST_IS_CUBIE := n
endif
