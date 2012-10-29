ifeq ($(TARGET),ANDROID)
# Android must use OpenGL
ENABLE_SDL = n
# UNIX/Linux defaults to OpenGL, but can use SDL_gfx instead
else ifeq ($(HAVE_WIN32),y)
# Windows defaults to GDI
ENABLE_SDL ?= n
else
# everything else must use SDL
ENABLE_SDL = y
endif

ifeq ($(ENABLE_SDL),y)
ifeq ($(TARGET),UNIX)
$(eval $(call pkg-config-library,SDL,sdl SDL_image))

ifndef SDL_TTF_LDLIBS
$(eval $(call pkg-config-library,SDL_TTF,SDL_ttf))
endif
SDL_LDLIBS += $(SDL_TTF_LDLIBS)

else
SDL_CPPFLAGS := -I/usr/local/i686-w64-mingw32/include/SDL
SDL_LDLIBS := -L/usr/local/i686-w64-mingw32/lib -lSDL -lSDL_image
endif

SDL_CPPFLAGS += -DENABLE_SDL
ifeq ($(OPENGL),n)
SDL_LDLIBS += -lSDL_gfx
endif # !OPENGL
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
