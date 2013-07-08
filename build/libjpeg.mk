LIBJPEG ?= n

ifeq ($(LIBJPEG),y)

ifeq ($(TARGET_IS_KOBO),y)
LIBJPEG_LDADD = $(KOBO)/lib/libjpeg.a
else
LIBJPEG_LDLIBS = -ljpeg
endif

endif
