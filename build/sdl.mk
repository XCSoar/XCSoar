ifeq ($(TARGET),ANDROID)
# Android must use OpenGL
ENABLE_SDL = n
else ifeq ($(HAVE_WIN32),y)
# Windows defaults to GDI
ENABLE_SDL ?= n
else ifeq ($(TARGET_IS_KOBO),y)
# the Kobo uses the frame buffer
ENABLE_SDL ?= n
else
# everything else defaults to SDL
ENABLE_SDL ?= y
endif

ifeq ($(ENABLE_SDL),y)

ifeq ($(TARGET_IS_DARWIN),y)
COREGRAPHICS = y
else
LIBPNG = y
LIBJPEG = y
endif
FREETYPE = y

$(eval $(call pkg-config-library,SDL,sdl2))
SDL_CPPFLAGS += -DENABLE_SDL

endif
