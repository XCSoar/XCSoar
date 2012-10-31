LIBPNG ?= n

ifeq ($(LIBPNG),y)

$(eval $(call pkg-config-library,LIBPNG,libpng))
LIBPNG_CPPFLAGS += -DUSE_LIBPNG

endif
