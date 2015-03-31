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

# Currently the default is not to use SDL2, but SDL 1.2, except for iOS,
# where no official SDL 1.2 release is available. And for OS X, because
# SDL 1.2 has compatibility issues with recent OSX SDK versions.
ifeq ($(TARGET_IS_DARWIN),y)
USE_SDL2 ?= y
else
USE_SDL2 ?= n
endif

ifeq ($(TARGET_IS_DARWIN),y)
COREGRAPHICS = y
else
LIBPNG = y
LIBJPEG = y
endif
FREETYPE = y

ifeq ($(USE_SDL2),y)
$(eval $(call pkg-config-library,SDL,sdl2))
SDL_CPPFLAGS := $(patsubst -I%,-isystem %,$(SDL_CPPFLAGS))
else
$(eval $(call pkg-config-library,SDL,sdl))
endif

SDL_CPPFLAGS += -DENABLE_SDL

endif
