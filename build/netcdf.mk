NETCDF = y

ifneq ($(TARGET),ANDROID)
$(eval $(call pkg-config-library,NETCDF,netcdf-cxx4))
$(eval $(call link-library,netcdfcpp,NETCDF))
NETCDF_LDLIBS = -lnetcdf_c++4 -lnetcdf
else
NETCDF_LDLIBS += -l:libnetcdf_c++.a -l:libnetcdf.a
endif
LDLIBS += $(NETCDF_LDLIBS)
