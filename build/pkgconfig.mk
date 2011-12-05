PKG_CONFIG = pkg-config

ifeq ($(TARGET_IS_DARWIN),y)
  PKG_CONFIG += --static
endif
