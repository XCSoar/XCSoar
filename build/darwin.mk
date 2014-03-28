ifeq ($(TARGET_IS_DARWIN),y)

TARGET_CPPFLAGS += -isystem $(DARWIN_LIBS)/include

TARGET_LDFLAGS += -framework CoreLocation

TARGET_ARCH += -isysroot $(DARWIN_SDK)

# Workaround for a clang bug (at least in clang versions 3.3 and 3.4):
# clang complains about an invalid -mlinker-version parameter when cross
# compiling for Mac OS X / iOS if this parameter is not used. So we
# explicitly define the -mlinker-version with the actual cross linker version.
ifeq ($(HOST_IS_DARWIN),n)
$(eval TARGET_ARCH += -mlinker-version=$(shell $(LD) -v 2>&1 | awk 'NR==1 { print $1 }'))
endif

endif

