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

endif
