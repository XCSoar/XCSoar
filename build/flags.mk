CXX_FEATURES = -fno-exceptions -fno-rtti
CXX_FEATURES += -std=gnu++0x
CXX_FEATURES += -fno-threadsafe-statics
CXX_FEATURES += -fmerge-all-constants

ifeq ($(TARGET),ANDROID)
# Temporary workaround for a libstdc++ 4.4 bug.  Disable this as soon
# as we have an Android NDK with gcc 4.5+.
CXX_FEATURES += -frtti
endif

ifeq ($(CLANG),n)
CXX_FEATURES += -fconserve-space -fno-operator-names
endif

ifneq ($(TARGET),WINE)
C_FEATURES = -std=gnu99
else
# libwine fails with -std=gnu99 due to funny "extern inline" tricks in
# winnt.h
C_FEATURES =
endif

ifeq ($(HAVE_WIN32),n)
CXX_FEATURES += -fvisibility=hidden
C_FEATURES += -fvisibility=hidden
endif

ifeq ($(DEBUG)$(HAVE_WIN32),nn)
CXX_FEATURES += -ffunction-sections
C_FEATURES += -ffunction-sections
TARGET_LDFLAGS += -Wl,--gc-sections
endif

ALL_CPPFLAGS = $(TARGET_INCLUDES) $(INCLUDES) $(TARGET_CPPFLAGS) $(CPPFLAGS)
ALL_CXXFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(CXX_FEATURES) $(CXXFLAGS)
ALL_CFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(C_FEATURES) $(CFLAGS)
