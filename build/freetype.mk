FREETYPE ?= n

ifeq ($(FREETYPE),y)

ifeq ($(TARGET_IS_KOBO),y)
FREETYPE_CPPFLAGS = -isystem $(KOBO)/include/freetype2
FREETYPE_LDADD = $(KOBO)/lib/libfreetype.a
else
$(eval $(call pkg-config-library,FREETYPE,freetype2))
endif

FREETYPE_CPPFLAGS += -DUSE_FREETYPE

endif
