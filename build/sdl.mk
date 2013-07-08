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

LIBPNG = y
LIBJPEG = y
FREETYPE = y

SDL_PKG = sdl

$(eval $(call pkg-config-library,SDL,$(SDL_PKG)))

SDL_CPPFLAGS += -DENABLE_SDL

ifeq ($(TARGET_IS_DARWIN),y)
# the pkg-config file on MacPorts is broken, we must convert all -l
# flags to link static libraries instead
SDL_LDADD := $(patsubst -l%,/opt/local/lib/lib%.a,$(filter -l%,$(SDL_LDLIBS)))
SDL_LDLIBS := $(filter-out -l% -R% -L%,$(SDL_LDLIBS))

SDL_LDADD += /opt/local/lib/libbz2.a /opt/local/lib/libz.a
SDL_LDADD += /opt/local/lib/libfreetype.a
SDL_LDADD += /opt/local/lib/libxcb.a /opt/local/lib/libXau.a /opt/local/lib/libXdmcp.a
endif
endif
