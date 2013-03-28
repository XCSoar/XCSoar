LIBJPEG ?= n

ifeq ($(LIBJPEG),y)

LIBJPEG_LDLIBS = -ljpeg
LIBJPEG_CPPFLAGS =

endif
