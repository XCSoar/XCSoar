LIBJPEG ?= n

ifeq ($(LIBJPEG),y)
$(eval $(call pkg-config-library,LIBJPEG,libjpeg))
endif
