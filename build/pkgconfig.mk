PKG_CONFIG = pkg-config

ifeq ($(TARGET),UNIX)
  ifeq ($(UNAME_S),Darwin)
    PKG_CONFIG += --static
  endif
endif
