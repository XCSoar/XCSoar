LLVM_PREFIX ?=
LLVM_SUFFIX ?=

LLVM_LINK = $(LLVM_PREFIX)llvm-link$(LLVM_SUFFIX)
LLVM_OPT = $(LLVM_PREFIX)opt$(LLVM_SUFFIX)
LLVM_LLC = $(LLVM_PREFIX)llc$(LLVM_SUFFIX)

ifeq ($(CLANG),y)

ifneq ($(TARGET),ANDROID)
ifneq ($(ANALYZER),y)
CXX = $(LLVM_PREFIX)clang++$(LLVM_SUFFIX)
CC = $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
endif

HOSTCC = $(CC)
HOSTCXX = $(CXX)
endif

DEPFLAGS = -MD -MF $(DEPFILE) -MT $@

ifeq ($(DEBUG)$(LLVM)$(LTO),nny)
AR += --plugin /usr/local/lib/LLVMgold.so
endif

ifeq ($(USE_CCACHE),y)
  # ccache will not use the optimisation of avoiding the 2nd call to the
  # pre-processor by compiling the pre-processed output that was used for
  # finding the hash in the case of a cache miss.
  CCACHE := export CCACHE_CPP2=yes && $(CCACHE)
endif

ifeq ($(TARGET),ANDROID)
  TARGET_ARCH := $(filter-out -mthumb-interwork,$(TARGET_ARCH))
  TARGET_ARCH += -gcc-toolchain $(ANDROID_GCC_TOOLCHAIN)
  TARGET_ARCH += -target $(LLVM_TRIPLE)
  TARGET_ARCH += -integrated-as
  TARGET_CPPFLAGS += -DBIONIC -DLIBCPP_NO_IOSTREAM
  TARGET_LDLIBS += -latomic
endif # Android

ifeq ($(TARGET_IS_PI),y)
  TARGET_LLVM_FLAGS = -march=arm -mcpu=arm1136jf-s -mattr=+vfp2 -float-abi=hard \
	-enable-no-infs-fp-math -enable-no-nans-fp-math -enable-unsafe-fp-math
  TARGET_ARCH += -target armv6-none-linux-gnueabihf -integrated-as
endif

ifeq ($(HOST_IS_PI)$(TARGET_IS_PI),ny)
  TARGET_CPPFLAGS += -isystem /opt/pi/root/usr/include/c++/4.6
  TARGET_CPPFLAGS += -isystem /opt/pi/root/usr/include/c++/4.6/arm-linux-gnueabihf
  TARGET_LDFLAGS += -L$(PI)/usr/lib/gcc/arm-linux-gnueabihf/4.6
endif

endif
