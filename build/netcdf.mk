HAVE_SKYSIGHT_NETCDF := n

# SkySight forecast decode needs both NetCDF and GeoTIFF support.
ifeq ($(HAVE_HTTP)$(GEOTIFF),yy)
ifneq ($(filter y,$(HAVE_WIN32) $(TARGET_IS_ANDROID) $(TARGET_IS_DARWIN)),)

ifeq ($(TARGET_IS_OSX),y)
# Homebrew's static netcdf.pc leaks CMake target names (e.g. HDF5::HDF5)
# into the linker arguments.  The macOS package is shared, so use its normal
# link interface instead.
$(eval $(call pkg-config-library-unstatic,LIBNETCDF,netcdf))
else
$(eval $(call pkg-config-library,LIBNETCDF,netcdf))
endif

HAVE_SKYSIGHT_NETCDF := y
NETCDF_CPPFLAGS += $(LIBNETCDF_CPPFLAGS) -DHAVE_SKYSIGHT_NETCDF
NETCDF_LDLIBS += $(LIBNETCDF_LDLIBS)

else ifeq ($(HOST_IS_LINUX)$(TARGET_IS_LINUX),yy)
ifeq ($(HOST_TRIPLET),)

$(eval $(call pkg-config-library,LIBNETCDF,netcdf))

HAVE_SKYSIGHT_NETCDF := y
NETCDF_CPPFLAGS += $(LIBNETCDF_CPPFLAGS) -DHAVE_SKYSIGHT_NETCDF
NETCDF_LDLIBS += $(LIBNETCDF_LDLIBS)

endif
endif
endif
