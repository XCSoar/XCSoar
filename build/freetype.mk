FREETYPE ?= n

ifeq ($(FREETYPE),y)
# minimum libtool number. For version and release numbers see https://github.com/aseprite/freetype2/blob/master/docs/VERSIONS.TXT
$(eval $(call pkg-config-library,FREETYPE,freetype2 '>=' 23.1.17))

FREETYPE_FEATURE_CPPFLAGS = -DUSE_FREETYPE

endif
