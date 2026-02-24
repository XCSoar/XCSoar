LLVM_SUFFIX ?=

ifeq ($(CLANG),y)

DEPFLAGS = -MD -MP -MF $(DEPFILE) -MT $@

ifneq ($(LLVM_TARGET),)
  TARGET_ARCH += -target $(LLVM_TARGET)
endif

ifeq ($(TARGET),ANDROID)
  TARGET_ARCH := $(filter-out -mthumb-interwork,$(TARGET_ARCH))
  TARGET_CPPFLAGS += -DBIONIC -DLIBCPP_NO_IOSTREAM
  TARGET_LDLIBS += -latomic
endif # Android

ifeq ($(TARGET_IS_PI),y)
  TARGET_LLVM_FLAGS = -march=arm -mcpu=arm1136jf-s -mattr=+vfp2 -float-abi=hard \
	-enable-no-infs-fp-math -enable-no-nans-fp-math -enable-unsafe-fp-math
endif

ifeq ($(HOST_IS_PI)$(TARGET_IS_PI),ny)
  TARGET_CPPFLAGS += -isystem /opt/pi/root/usr/include/c++/4.6
  TARGET_CPPFLAGS += -isystem /opt/pi/root/usr/include/c++/4.6/arm-linux-gnueabihf
  TARGET_LDFLAGS += -L$(PI)/usr/lib/gcc/arm-linux-gnueabihf/4.6
endif

ifeq ($(HOST_IS_ARM)$(ARMV7)$(TARGET_IS_CUBIE),nyy)
  # cross-crompiling for Cubieboard
  TARGET_LLVM_FLAGS = -march=arm -mcpu=cortex-a8 -mattr=+neon -float-abi=hard \
	-enable-no-infs-fp-math -enable-no-nans-fp-math -enable-unsafe-fp-math
  TARGET_CPPFLAGS += -isystem $(CUBIE)/usr/include/c++/4.7
  TARGET_CPPFLAGS += -isystem $(CUBIE)/usr/include/c++/4.7/arm-linux-gnueabihf
  TARGET_LDFLAGS += -L$(CUBIE)/usr/lib/gcc/arm-linux-gnueabihf/4.7
endif

endif
