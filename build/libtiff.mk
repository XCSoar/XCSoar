# Experimental feature - only enabled on Linux for now
ifeq ($(TARGET)$(TARGET_IS_KOBO),UNIXn)
GEOTIFF ?= y
else ifeq ($(TARGET),ANDROID)
GEOTIFF ?= y
else
GEOTIFF ?= n
endif

TIFF ?= $(GEOTIFF)

ifeq ($(TIFF),y)

$(eval $(call pkg-config-library,LIBTIFF,libtiff-4))
LIBTIFF_CPPFLAGS += -DUSE_LIBTIFF

ifeq ($(GEOTIFF),y)
LIBTIFF_CPPFLAGS += -DUSE_GEOTIFF
ifneq ($(USE_THIRDPARTY_LIBS),y)
LIBTIFF_CPPFLAGS += -isystem /usr/include/geotiff
endif
LIBTIFF_LDLIBS += -lgeotiff
endif

ifeq ($(GEOTIFF)$(USE_THIRDPARTY_LIBS),yy)
$(eval $(call pkg-config-library,PROJ,proj))
LIBTIFF_CPPFLAGS += $(PROJ_CPPFLAGS)
LIBTIFF_LDLIBS += $(PROJ_LDLIBS)
endif

endif
