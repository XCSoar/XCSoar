ifeq ($(MAKECMDGOALS),kobo-libs) # kludge to allow bootstrapping kobo-libs
$(error Target "kobo-libs" is obsolete, please use "libs" instead)
endif

ifeq ($(TARGET_IS_KOBO),y)
USE_THIRDARTY_LIBS = y
else ifeq ($(TARGET),PC)
USE_THIRDARTY_LIBS = y
else ifeq ($(TARGET),ANDROID)
USE_THIRDARTY_LIBS = y
else ifeq ($(TARGET_IS_DARWIN),y)
USE_THIRDARTY_LIBS = y
else
USE_THIRDARTY_LIBS = n
endif

ifeq ($(USE_THIRDARTY_LIBS),y)

.PHONY: libs
libs:
	./build/thirdparty.py $(TARGET_OUTPUT_DIR) $(TARGET) $(HOST_TRIPLET) "$(TARGET_ARCH)" "$(TARGET_CPPFLAGS)" $(CC) $(CXX) $(AR) $(STRIP)

THIRDARTY_LIBS_ROOT = $(TARGET_OUTPUT_DIR)/lib/$(HOST_TRIPLET)/root
TARGET_CPPFLAGS += -isystem $(THIRDARTY_LIBS_ROOT)/include
TARGET_LDFLAGS += -L$(THIRDARTY_LIBS_ROOT)/lib

PKG_CONFIG := PKG_CONFIG_LIBDIR=$(THIRDARTY_LIBS_ROOT)/lib/pkgconfig $(PKG_CONFIG)

endif
