LIBJPEG ?= n

ifeq ($(LIBJPEG),y)
LIBJPEG_LDLIBS = -ljpeg
endif
