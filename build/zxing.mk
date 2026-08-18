# zxing-cpp decodes the QR codes scanned by the task QR scanner.  Only
# the Android port has a camera to feed it so far; the decoder itself is
# portable, so this is deliberately not guarded by anything but
# availability.
ZXING ?= $(TARGET_IS_ANDROID)

ifeq ($(ZXING),y)

$(eval $(call pkg-config-library,ZXING,zxing))

ZXING_CPPFLAGS += -DHAVE_ZXING

endif
