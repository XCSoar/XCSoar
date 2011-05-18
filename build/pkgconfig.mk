PKG_CONFIG = pkg-config

ifeq ($(TARGET),UNIX)
  ifeq ($(shell uname -s),Darwin)
    PKG_CONFIG += --static
  endif
endif
