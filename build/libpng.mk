LIBPNG ?= n

ifeq ($(LIBPNG),y)

$(eval $(call pkg-config-library,LIBPNG,libpng))

endif
