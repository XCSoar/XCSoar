ENABLE_SDL ?= $(call bool_not,$(HAVE_WIN32))

ifeq ($(ENABLE_SDL),y)
ifeq ($(TARGET),UNIX)
SDL_CPPFLAGS := $(shell pkg-config --cflags sdl 2>/dev/null)
SDL_LDLIBS := $(shell pkg-config --libs sdl 2>/dev/null)
else
ifeq ($(TARGET),ANDROID)
SDL_CPPFLAGS := -I/usr/local/android-arm/include/SDL
SDL_LDLIBS := -L/usr/local/android-arm/lib -lsdl-1.2 -lsdl_gfx -lsdl_ttf
else
SDL_CPPFLAGS := -I/usr/local/i586-mingw32msvc/include/SDL
SDL_LDLIBS := -L/usr/local/i586-mingw32msvc/lib -lSDL
endif
endif

SDL_CPPFLAGS += -DENABLE_SDL
ifneq ($(TARGET),ANDROID)
SDL_LDLIBS += -lSDL_gfx -lSDL_ttf
endif
endif
