FREETYPE ?= n

ifeq ($(FREETYPE),y)

$(eval $(call pkg-config-library,FREETYPE,freetype2))
FREETYPE_CPPFLAGS := $(patsubst -I%,-isystem %,$(FREETYPE_CPPFLAGS))

FREETYPE_FEATURE_CPPFLAGS = -DUSE_FREETYPE

endif
