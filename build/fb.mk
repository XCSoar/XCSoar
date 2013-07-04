USE_FB = $(TARGET_IS_KOBO)

ifeq ($(USE_FB),y)
USE_CONSOLE = y
USE_MEMORY_CANVAS = y
FREETYPE = y
EGL = n
OPENGL = n
ENABLE_SDL = n

ifeq ($(TARGET_IS_KOBO),y)
SDL_LDADD += $(KOBO)/lib/libpng.a $(KOBO)/lib/libjpeg.a
SDL_LDADD += $(KOBO)/lib/libfreetype.a
endif

endif
