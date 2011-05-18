ENABLE_SDL ?= $(call bool_not,$(HAVE_WIN32))

ifeq ($(ENABLE_SDL),y)
ifeq ($(TARGET),UNIX)
OPENGL ?= y
SDL_CPPFLAGS := $(shell $(PKG_CONFIG) --cflags sdl 2>/dev/null)
SDL_LDLIBS := $(shell $(PKG_CONFIG) --libs sdl 2>/dev/null)
else
ifeq ($(TARGET),ANDROID)
OPENGL = y
SDL_CPPFLAGS :=
SDL_LDLIBS :=
else
OPENGL ?= n
SDL_CPPFLAGS := -I/usr/local/i586-mingw32msvc/include/SDL
SDL_LDLIBS := -L/usr/local/i586-mingw32msvc/lib -lSDL
endif
endif

SDL_CPPFLAGS += -DENABLE_SDL
ifeq ($(OPENGL),y)
SDL_CPPFLAGS += -DENABLE_OPENGL
ifneq ($(TARGET),ANDROID)
ifeq ($(shell uname -s),Darwin)
SDL_LDLIBS += -framework OpenGL
else
SDL_LDLIBS += -lGL
endif
endif
else # !OPENGL
SDL_LDLIBS += -lSDL_gfx
endif # !OPENGL
ifneq ($(TARGET),ANDROID)
SDL_LDLIBS += -lSDL_ttf
endif
endif
