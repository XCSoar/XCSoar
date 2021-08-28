CXX_FEATURES = -std=c++20
CXX_FEATURES += -fno-threadsafe-statics
CXX_FEATURES += -fmerge-all-constants

ifeq ($(CLANG),n)
CXX_FEATURES += -fconserve-space -fno-operator-names
endif

C_FEATURES = -std=c11

# produce position independent code when compiling the python library
ifeq ($(MAKECMDGOALS),python)
CXX_FEATURES += -fPIC
C_FEATURES += -fPIC
LDFLAGS += -fPIC -shared
endif

ifeq ($(ICF),y)
  LDFLAGS += -Wl,--icf=all

  ifeq ($(CLANG),y)
    USE_LD = lld
  else
    USE_LD = gold
  endif

  ifneq ($(TARGET_IS_PI)$(TARGET_IS_PI4),yn)
    # Hide all symbols from static libraries we link with; this has a
    # huge effect on Android where libc++'s symbols are exported by
    # default.
    LDFLAGS += -Wl,--exclude-libs,ALL

    # Note that the above "ifneq" exclude the Raspberry Pi 1-3 from
    # this because the Broadcom VideoCore library apparently needs to
    # export symbols from the static library to libEGL; hiding those
    # symbols would break rendering.
  endif
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
ALL_CXXFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(SANITIZE_FLAGS) $(CXX_FEATURES) $(TARGET_CXXFLAGS) $(CXX_WARNINGS) $(CXXFLAGS) $(EXTRA_CXXFLAGS)
ALL_CFLAGS = $(OPTIMIZE) $(FLAGS_PROFILE) $(SANITIZE_FLAGS) $(C_FEATURES) $(C_WARNINGS) $(CFLAGS) $(EXTRA_CFLAGS)

ALL_LDFLAGS = $(OPTIMIZE_LDFLAGS) $(TARGET_LDFLAGS) $(FLAGS_PROFILE) $(SANITIZE_FLAGS) $(LDFLAGS) $(EXTRA_LDFLAGS)
ALL_LDLIBS = $(TARGET_LDLIBS) $(COVERAGE_LDLIBS) $(LDLIBS) $(EXTRA_LDLIBS)
