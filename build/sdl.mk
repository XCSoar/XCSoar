ENABLE_SDL ?= $(call bool_not,$(HAVE_WIN32))

ifeq ($(ENABLE_SDL),y)
ifeq ($(TARGET),UNIX)
SDL_CPPFLAGS := $(shell pkg-config --cflags sdl)
SDL_LDLIBS := $(shell pkg-config --libs sdl)
else
SDL_CPPFLAGS := -I/usr/local/i586-mingw32msvc/include
SDL_LDLIBS := -L/usr/local/i586-mingw32msvc/lib -lSDL
endif

SDL_CPPFLAGS += -DENABLE_SDL
SDL_LDLIBS += -lSDL_gfx -lSDL_ttf
endif
