# Experimental feature - only enabled on Linux for now
ifeq ($(TARGET)$(TARGET_IS_KOBO),UNIXn)
TIFF ?= y
else
TIFF ?= n
endif

ifeq ($(TIFF),y)

$(eval $(call pkg-config-library,LIBTIFF,libtiff-4))
LIBTIFF_CPPFLAGS := $(patsubst -I%,-isystem %,$(LIBTIFF_CPPFLAGS)) -DUSE_LIBTIFF

endif
