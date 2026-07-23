# GeoTIFF-backed raster overlays are available on all non-Kobo targets.
ifeq ($(TARGET_IS_KOBO),y)
GEOTIFF = n
else ifeq ($(TARGET_IS_DARWIN),y)
GEOTIFF ?= y
else ifeq ($(TARGET),PC)
GEOTIFF ?= y
else ifeq ($(TARGET)$(TARGET_IS_KOBO),UNIXn)
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
LIBGEOTIFF_USE_PKG_CONFIG := y
LIBGEOTIFF_LDLIBS = -lgeotiff

ifneq ($(USE_THIRDPARTY_LIBS),y)
ifeq ($(HOST_IS_LINUX)$(TARGET_IS_LINUX),yy)
ifeq ($(HOST_TRIPLET),)
# Native Linux distributions may ship libgeotiff without libgeotiff.pc.
LIBGEOTIFF_USE_PKG_CONFIG := n
endif
endif
endif

ifeq ($(LIBGEOTIFF_USE_PKG_CONFIG),y)
$(eval $(call pkg-config-library,LIBGEOTIFF,libgeotiff))
LIBTIFF_CPPFLAGS += $(LIBGEOTIFF_CPPFLAGS)
else
ifneq ($(shell $(PKG_CONFIG) --exists proj >/dev/null 2>&1 && echo y),)
# Some system libgeotiff builds don't ship libgeotiff.pc, but still
# depend on PROJ at link time (e.g. libgeotiff 1.7.x on Yocto).
$(eval $(call pkg-config-library,LIBGEOTIFF_PROJ,proj))
LIBTIFF_LDLIBS += $(LIBGEOTIFF_PROJ_LDLIBS)
endif
endif

LIBTIFF_LDLIBS += $(LIBGEOTIFF_LDLIBS)
endif

ifeq ($(GEOTIFF)$(USE_THIRDPARTY_LIBS),yy)
$(eval $(call pkg-config-library,PROJ,proj))
LIBTIFF_CPPFLAGS += $(PROJ_CPPFLAGS)
LIBTIFF_LDLIBS += $(PROJ_LDLIBS)
endif

endif
