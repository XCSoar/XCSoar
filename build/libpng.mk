LIBPNG ?= n

ifeq ($(LIBPNG),y)

ifeq ($(TARGET_IS_KOBO),y)
LIBPNG_LDADD = $(KOBO)/lib/libpng.a
else
$(eval $(call pkg-config-library,LIBPNG,libpng))
endif

endif
