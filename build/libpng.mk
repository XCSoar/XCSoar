LIBPNG ?= n

ifeq ($(LIBPNG),y)

$(eval $(call pkg-config-library,LIBPNG,libpng))

LIBPNG_LDADD += $(ZLIB_LDADD)
LIBPNG_LDLIBS += $(ZLIB_LDLIBS)

endif
