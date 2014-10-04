ENABLE_UDEV ?= n

ifeq ($(ENABLE_UDEV),y)

$(eval $(call pkg-config-library,UDEV,libudev))
UDEV_CPPFLAGS := $(patsubst -I%,-isystem %,$(UDEV_CPPFLAGS))

endif
