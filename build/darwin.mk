ifeq ($(TARGET_IS_DARWIN),y)

TARGET_LDLIBS += -framework Foundation
TARGET_LDLIBS += -framework AVFoundation
ifeq ($(TARGET_IS_IOS),y)
ifneq ($(wildcard $(DARWIN_SDK)/System/Library/Frameworks/Network.framework),)
TARGET_CPPFLAGS += -DHAVE_APPLE_NETWORK_FRAMEWORK
TARGET_LDLIBS += -weak_framework Network
endif
else
TARGET_CPPFLAGS += -DHAVE_APPLE_NETWORK_FRAMEWORK
TARGET_LDLIBS += -framework Network
endif

ifneq ($(DARWIN_LIBS),)
TARGET_CPPFLAGS += -isystem $(DARWIN_LIBS)/include
endif
TARGET_CXXFLAGS += -ObjC++ -fobjc-arc

ifeq ($(TARGET_IS_IOS),y)
DARWIN_CXX_INCLUDE ?= $(shell xcrun --sdk iphoneos --show-sdk-path 2>/dev/null)/usr/include/c++/v1
ifneq ($(wildcard $(DARWIN_CXX_INCLUDE)/algorithm),)
TARGET_CXXFLAGS += -isystem $(DARWIN_CXX_INCLUDE)
endif
endif

# 32-bit iOS is linked against an old SDK libc++ that predates libc++'s
# per-symbol ABI tags.  Disable them so the mangled names match.
ifeq ($(TARGET_IS_IOS)$(HOST_TRIPLET),yarmv7-apple-darwin)
TARGET_CXXFLAGS += -D_LIBCPP_NO_ABI_TAG
endif

ifneq ($(DARWIN_SDK),)
TARGET_ARCH += -isysroot $(DARWIN_SDK)
endif

endif
