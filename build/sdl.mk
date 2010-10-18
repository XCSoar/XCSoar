ENABLE_SDL ?= $(call bool_not,$(HAVE_WIN32))

ifeq ($(ENABLE_SDL),y)
ifeq ($(TARGET),UNIX)
SDL_CPPFLAGS := $(shell pkg-config --cflags sdl 2>/dev/null)
SDL_LDLIBS := $(shell pkg-config --libs sdl 2>/dev/null)
else
SDL_CPPFLAGS := -I/usr/local/i586-mingw32msvc/include/SDL
SDL_LDLIBS := -L/usr/local/i586-mingw32msvc/lib -lSDL
endif

SDL_CPPFLAGS += -DENABLE_SDL
SDL_LDLIBS += -lSDL_gfx -lSDL_ttf
endif
