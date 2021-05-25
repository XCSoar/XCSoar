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

# In most cases, the ACTUAL_HOST_TRIPLET is not explicitly set. Set it to the
# value of HOST_TRIPLET in this case.
# This is for targets for which a toolchain is used, which was originally
# intended for another target ABI, e. g. when the arm-linux-gnueabihf toolchain
# used to compile for musl instead of glibc, and so the actual triplet is
# something like arm-linux-musleabihf.
ifeq ($(ACTUAL_HOST_TRIPLET),)
  ACTUAL_HOST_TRIPLET = $(HOST_TRIPLET)
endif

# -Wl,--gc-sections breaks the (Kobo) glibc build
THIRDPARTY_LDFLAGS_FILTER_OUT = -L$(THIRDPARTY_LIBS_DIR)/% -Wl,--gc-sections

THIRDPARTY_LIBS_DIR = $(TARGET_OUTPUT_DIR)/lib
THIRDPARTY_LIBS_ROOT = $(THIRDPARTY_LIBS_DIR)/$(ACTUAL_HOST_TRIPLET)

.PHONY: libs
libs: $(THIRDPARTY_LIBS_DIR)/stamp

compile-depends += $(THIRDPARTY_LIBS_DIR)/stamp
$(THIRDPARTY_LIBS_DIR)/stamp:
	./build/thirdparty.py $(TARGET_OUTPUT_DIR) $(TARGET) $(HOST_TRIPLET) $(ACTUAL_HOST_TRIPLET) "$(TARGET_ARCH)" "$(TARGET_CPPFLAGS)" "$(filter-out $(THIRDPARTY_LDFLAGS_FILTER_OUT),$(TARGET_LDFLAGS))" $(CC) $(CXX) $(AR) "$(ARFLAGS)" $(RANLIB) $(STRIP) "$(WINDRES)"
	touch $@

ifeq ($(TARGET_IS_KOBO),n)
TARGET_CPPFLAGS += -isystem $(THIRDPARTY_LIBS_ROOT)/include
TARGET_LDFLAGS += -L$(THIRDPARTY_LIBS_ROOT)/lib
endif

endif

compile-depends += boost
