# for now, udev is only used for the libinput event driver
ENABLE_UDEV ?= $(USE_LIBINPUT)

ifeq ($(ENABLE_UDEV),y)

$(eval $(call pkg-config-library,UDEV,libudev))

endif
