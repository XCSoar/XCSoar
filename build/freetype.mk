FREETYPE ?= $(GREYSCALE)

ifeq ($(FREETYPE),y)

$(eval $(call pkg-config-library,FREETYPE,freetype2))
FREETYPE_CPPFLAGS += -DUSE_FREETYPE

endif
