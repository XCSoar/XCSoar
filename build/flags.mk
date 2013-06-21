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

ifeq ($(HAVE_WIN32),n)
CXX_FEATURES += -fvisibility=hidden
C_FEATURES += -fvisibility=hidden
endif

ifeq ($(DEBUG)$(HAVE_WIN32)$(TARGET_IS_DARWIN),nnn)
CXX_FEATURES += -ffunction-sections
C_FEATURES += -ffunction-sections
TARGET_LDFLAGS += -Wl,--gc-sections
endif

ifeq ($(DEBUG),y)
CXX_FEATURES += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
endif

ALL_CPPFLAGS = $(TARGET_INCLUDES) $(INCLUDES) $(TARGET_CPPFLAGS) $(CPPFLAGS) $(EXTRA_CPPFLAGS)
ALL_CXXFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(CXX_FEATURES) $(CXXFLAGS) $(EXTRA_CXXFLAGS)
ALL_CFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(C_FEATURES) $(CFLAGS) $(EXTRA_CFLAGS)

ALL_LDFLAGS = $(filter-out -emit-llvm,$(OPTIMIZE)) $(TARGET_LDFLAGS) $(FLAGS_PROFILE) $(LDFLAGS)
ALL_LDLIBS = $(TARGET_LDLIBS) $(COVERAGE_LDLIBS) $(LDLIBS)
