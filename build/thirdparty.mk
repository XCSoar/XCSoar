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

THIRDPARTY_LIBS_DIR = $(TARGET_OUTPUT_DIR)/lib/$(HOST_TRIPLET)
THIRDPARTY_LIBS_ROOT = $(THIRDPARTY_LIBS_DIR)/root

.PHONY: libs
libs: $(THIRDPARTY_LIBS_DIR)/stamp

compile-depends += $(THIRDPARTY_LIBS_DIR)/stamp
$(THIRDPARTY_LIBS_DIR)/stamp:
	./build/thirdparty.py $(TARGET_OUTPUT_DIR) $(TARGET) $(HOST_TRIPLET) "$(TARGET_ARCH)" "$(TARGET_CPPFLAGS)" "$(filter-out $(THIRDPARTY_LDFLAGS_FILTER_OUT),$(TARGET_LDFLAGS))" $(CC) $(CXX) $(AR) $(RANLIB) $(STRIP)
	touch $@

TARGET_CPPFLAGS += -isystem $(THIRDPARTY_LIBS_ROOT)/include
TARGET_LDFLAGS += -L$(THIRDPARTY_LIBS_ROOT)/lib

endif

compile-depends += boost
