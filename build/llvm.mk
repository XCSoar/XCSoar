ifeq ($(CLANG),y)

ifneq ($(ANALYZER),y)
CXX = clang++
CC = clang
endif

DEPFLAGS = -MD -MF $(DEPFILE) -MT $@

HOSTCC = $(CC)
HOSTCXX = $(CXX)

ifneq ($(DEBUG),y)
OPTIMIZE := -O4 -DNDEBUG -Wuninitialized
TARGET_LDFLAGS += -use-gold-plugin
ARFLAGS += --plugin /usr/local/lib/LLVMgold.so
endif

ifeq ($(USE_CCACHE),y)
  # ccache will not use the optimisation of avoiding the 2nd call to the
  # pre-processor by compiling the pre-processed output that was used for
  # finding the hash in the case of a cache miss.
  CCACHE := export CCACHE_CPP2=yes && $(CCACHE)
endif

ifeq ($(TARGET),ANDROID)
  TARGET_ARCH := $(subst armv5te,armv5,$(TARGET_ARCH))
  TARGET_ARCH := $(filter-out -mthumb-interwork,$(TARGET_ARCH))
  TARGET_ARCH += -ccc-host-triple arm-android-eabi -integrated-as
  TARGET_CPPFLAGS += -DBIONIC -DLIBCPP_NO_IOSTREAM
  CXX_FEATURES := $(filter-out -frtti,$(CXX_FEATURES))
endif # Android

endif
