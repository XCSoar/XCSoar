CXX_FEATURES = -fno-exceptions -fno-rtti
CXX_FEATURES += -std=gnu++0x
CXX_FEATURES += -fno-threadsafe-statics
CXX_FEATURES += -fmerge-all-constants

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

ALL_CPPFLAGS = $(TARGET_INCLUDES) $(INCLUDES) $(TARGET_CPPFLAGS) $(CPPFLAGS)
ALL_CXXFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(CXX_FEATURES) $(CXXFLAGS)
ALL_CFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(C_FEATURES) $(CFLAGS)
