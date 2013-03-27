# Make library that auto-detects the build environment.

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

HOST_IS_LINUX := $(call string_equals,$(UNAME_S),Linux)
HOST_IS_DARWIN := $(call string_equals,$(UNAME_S),Darwin)
HOST_IS_CYGWIN := $(call string_equals,$(UNAME_S),Cygwin)
HOST_IS_MINGW := $(call string_contains,$(UNAME_S),MINGW)
HOST_IS_WIN32 := $(call bool_or,$(HOST_IS_CYGWIN),$(HOST_IS_MINGW))
HOST_IS_UNIX := $(call bool_not,$(HOST_IS_WIN32))

HOST_IS_ARMV6 := $(call string_equals,$(UNAME_M),armv6l)
HOST_IS_ARMV7 := $(call string_equals,$(UNAME_M),armv7l)

ifeq ($(HOST_IS_ARMV7),y)
HOST_HAS_NEON := $(call string_contains,$(shell grep -E ^Features /proc/cpuinfo),neon)
else
HOST_HAS_NEON := n
endif

ifeq ($(HOST_IS_LINUX)$(HOST_IS_ARMV6),yy)
# Check for VideoCore headers present on a Raspberry Pi
HOST_IS_PI := $(call string_equals,$(shell test -f /opt/vc/include/interface/vmcs_host/vc_dispmanx.h && echo y),y)
else
HOST_IS_PI := n
endif
