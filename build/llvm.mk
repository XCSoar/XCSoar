ifeq ($(CLANG),y)

ifneq ($(TARGET),ANDROID)
ifneq ($(ANALYZER),y)
CXX = clang++
CC = clang
endif

HOSTCC = $(CC)
HOSTCXX = $(CXX)
endif

DEPFLAGS = -MD -MF $(DEPFILE) -MT $@

ifeq ($(DEBUG)$(LLVM),yn)
OPTIMIZE := -O4 -DNDEBUG -Wuninitialized
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
  TARGET_ARCH += -ccc-host-triple $(LLVM_TRIPLE)
  TARGET_CPPFLAGS += -DBIONIC -DLIBCPP_NO_IOSTREAM
endif # Android

ifeq ($(HOST_IS_PI)$(TARGET_IS_PI),ny)
  TARGET_ARCH := -target armv6-none-linux-gnueabihf -mfloat-abi=hard -integrated-as
  TARGET_CPPFLAGS += -isystem /opt/pi/root/usr/include/c++/4.6
  TARGET_CPPFLAGS += -isystem /opt/pi/root/usr/include/c++/4.6/arm-linux-gnueabihf
  TARGET_LDFLAGS += -L$(PI)/usr/lib/gcc/arm-linux-gnueabihf/4.6
endif

endif
