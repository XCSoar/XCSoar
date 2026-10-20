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

ifeq ($(DO_CROSS_COMPILE)$(TARGET_IS_PI),yy)
  TARGET_CPPFLAGS += -isystem /opt/pi/root/usr/include/c++/4.6
  TARGET_CPPFLAGS += -isystem /opt/pi/root/usr/include/c++/4.6/arm-linux-gnueabihf
  ifneq ($(SYSROOT),)
    TARGET_LDFLAGS += -L$(SYSROOT)/usr/lib/gcc/arm-linux-gnueabihf/4.6
  endif
endif

ifeq ($(DO_CROSS_COMPILE)$(TARGET_IS_CUBIE),yy)
  # cross-crompiling for Cubieboard
  TARGET_LLVM_FLAGS = -march=arm -mcpu=cortex-a8 -mattr=+neon -float-abi=hard \
	-enable-no-infs-fp-math -enable-no-nans-fp-math -enable-unsafe-fp-math
  ifneq ($(SYSROOT),)
    TARGET_CPPFLAGS += -isystem $(SYSROOT)/usr/include/c++/4.7
    TARGET_CPPFLAGS += -isystem $(SYSROOT)/usr/include/c++/4.7/arm-linux-gnueabihf
    TARGET_LDFLAGS += -L$(SYSROOT)/usr/lib/gcc/arm-linux-gnueabihf/4.7
  endif
endif

endif
