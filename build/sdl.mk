ifeq ($(TARGET),ANDROID)
# Android must use OpenGL
ENABLE_SDL = n
# UNIX/Linux defaults to OpenGL, but can use SDL_gfx instead
else ifeq ($(HAVE_WIN32),y)
# Windows defaults to GDI
ENABLE_SDL ?= n
else
# everything else defaults to SDL
ENABLE_SDL ?= y
endif

ifeq ($(ENABLE_SDL),y)

LIBPNG = y
LIBJPEG = y

SDL_PKG = sdl

ifeq ($(FREETYPE),n)
SDL_PKG += SDL_ttf
endif

ifeq ($(OPENGL)$(GREYSCALE),nn)
SDL_PKG += SDL_gfx
endif

ifeq ($(TARGET_IS_KOBO),y)
SDL_CPPFLAGS += -isystem $(KOBO)/include/SDL
SDL_LDADD += $(KOBO)/lib/libpng.a $(KOBO)/lib/libjpeg.a
SDL_LDADD += $(KOBO)/lib/libSDL_ttf.a $(KOBO)/lib/libfreetype.a
SDL_LDADD += $(KOBO)/lib/libSDL.a
else
$(eval $(call pkg-config-library,SDL,$(SDL_PKG)))
endif

SDL_CPPFLAGS += -DENABLE_SDL

ifeq ($(OPENGL),n)
SDL_CPPFLAGS += -DUSE_SDL_GFX
endif

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
