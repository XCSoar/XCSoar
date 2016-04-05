ifeq ($(MAKECMDGOALS),kobo-libs) # kludge to allow bootstrapping kobo-libs
$(error Target "kobo-libs" is obsolete, please use "libs" instead)
endif

ifeq ($(TARGET_IS_KOBO),y)
USE_THIRDPARTY_LIBS = y
else ifeq ($(TARGET),PC)
USE_THIRDPARTY_LIBS = y
else ifeq ($(TARGET),ANDROID)
  ifeq ($(FAT_BINARY),y)
    # this is handled by android.mk
    USE_THIRDPARTY_LIBS = n
  else
    USE_THIRDPARTY_LIBS = y
  endif
else ifeq ($(TARGET_IS_DARWIN),y)
USE_THIRDPARTY_LIBS = y
else
USE_THIRDPARTY_LIBS = n
endif

ifeq ($(USE_THIRDPARTY_LIBS),y)

# -Wl,--gc-sections breaks the (Kobo) glibc build
THIRDPARTY_LDFLAGS_FILTER_OUT = -L% -Wl,--gc-sections

.PHONY: libs
libs: $(TARGET_OUTPUT_DIR)/lib/$(HOST_TRIPLET)/root/stamp

compile-depends += $(TARGET_OUTPUT_DIR)/lib/$(HOST_TRIPLET)/root/stamp
$(TARGET_OUTPUT_DIR)/lib/$(HOST_TRIPLET)/root/stamp:
	./build/thirdparty.py $(TARGET_OUTPUT_DIR) $(TARGET) $(HOST_TRIPLET) "$(TARGET_ARCH)" "$(TARGET_CPPFLAGS)" "$(filter-out $(THIRDPARTY_LDFLAGS_FILTER_OUT),$(TARGET_LDFLAGS))" $(CC) $(CXX) $(AR) $(STRIP)
	touch $@

THIRDPARTY_LIBS_ROOT = $(TARGET_OUTPUT_DIR)/lib/$(HOST_TRIPLET)/root
TARGET_CPPFLAGS += -isystem $(THIRDPARTY_LIBS_ROOT)/include
TARGET_LDFLAGS += -L$(THIRDPARTY_LIBS_ROOT)/lib

endif
