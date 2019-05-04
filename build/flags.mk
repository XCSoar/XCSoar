CXX_FEATURES = -std=gnu++17
CXX_FEATURES += -fno-threadsafe-statics
CXX_FEATURES += -fmerge-all-constants

ifeq ($(CLANG),n)
CXX_FEATURES += -fconserve-space -fno-operator-names
endif

C_FEATURES = -std=gnu99

# produce position independent code when compiling the python library
ifeq ($(MAKECMDGOALS),python)
CXX_FEATURES += -fPIC
C_FEATURES += -fPIC
LDFLAGS += -fPIC -shared
endif

ifeq ($(ICF),y)
LDFLAGS += -Wl,--icf=all
USE_LD = gold
endif

ifneq ($(USE_LD),)
LDFLAGS += -fuse-ld=$(USE_LD)
endif

ifneq ($(MAKECMDGOALS),python)
ifeq ($(HAVE_WIN32),n)
CXX_FEATURES += -fvisibility=hidden
C_FEATURES += -fvisibility=hidden
endif
endif

ifeq ($(DEBUG)$(HAVE_WIN32)$(TARGET_IS_DARWIN),nnn)
CXX_FEATURES += -ffunction-sections
C_FEATURES += -ffunction-sections
TARGET_LDFLAGS += -Wl,--gc-sections
endif

ALL_CPPFLAGS = $(TARGET_INCLUDES) $(INCLUDES) $(TARGET_CPPFLAGS) $(CPPFLAGS) $(EXTRA_CPPFLAGS)
ALL_CXXFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(SANITIZE_FLAGS) $(CXX_FEATURES) $(TARGET_CXXFLAGS) $(CXXFLAGS) $(EXTRA_CXXFLAGS)
ALL_CFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(SANITIZE_FLAGS) $(C_FEATURES) $(CFLAGS) $(EXTRA_CFLAGS)

ALL_LDFLAGS = $(OPTIMIZE_LDFLAGS) $(TARGET_LDFLAGS) $(FLAGS_PROFILE) $(SANITIZE_FLAGS) $(LDFLAGS) $(EXTRA_LDFLAGS)
ALL_LDLIBS = $(TARGET_LDLIBS) $(COVERAGE_LDLIBS) $(LDLIBS) $(EXTRA_LDLIBS)
