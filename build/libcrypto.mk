# Build rules for the crypto library

ifneq ($(TARGET_IS_DARWIN),y)
$(eval $(call pkg-config-library,LIBSODIUM,libsodium))
endif
