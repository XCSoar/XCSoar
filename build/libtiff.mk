# Experimental feature - only enabled on Linux for now
ifeq ($(TARGET)$(TARGET_IS_KOBO),UNIXn)
GEOTIFF ?= y
else
GEOTIFF ?= n
endif

TIFF ?= $(GEOTIFF)

ifeq ($(TIFF),y)

$(eval $(call pkg-config-library,LIBTIFF,libtiff-4))
LIBTIFF_CPPFLAGS := $(patsubst -I%,-isystem %,$(LIBTIFF_CPPFLAGS)) -DUSE_LIBTIFF

ifeq ($(GEOTIFF),y)
LIBTIFF_CPPFLAGS += -isystem /usr/include/geotiff -DUSE_GEOTIFF
LIBTIFF_LDLIBS += -lgeotiff
endif

endif
