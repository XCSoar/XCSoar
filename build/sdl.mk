ifeq ($(TARGET),ANDROID)
# Android must use OpenGL
ENABLE_SDL = n
else ifeq ($(HAVE_WIN32),y)
# OpenGL Windows flavors set ENABLE_SDL=y in targets.mk first.
# Bare TARGET=PC leaves SDL off and is rejected later.
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
ifeq ($(TARGET_IS_IOS),y)
UIKIT = y
else
APPKIT = y
# On macOS, SDL can work with ANGLE/OpenGL
# SDL will use the OpenGL context created with ANGLE
endif
else
LIBPNG = y
LIBJPEG = y
FREETYPE = y
endif

$(eval $(call pkg-config-library,SDL,sdl3 '>=' 3.2.10))

ifeq ($(HAVE_WIN32),y)
# XCSoar provides WinMain; SDL_main.h must not generate another entry point.
SDL_CPPFLAGS += -DENABLE_SDL -DSDL_MAIN_HANDLED

# Override the default "console" subsystem (set in targets.mk) to
# "windows" for GUI programs, to avoid console pop-up
SDL_LDLIBS += -Wl,-subsystem,windows
else
SDL_CPPFLAGS += -DENABLE_SDL
endif

endif
