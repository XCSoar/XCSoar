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

$(eval $(call pkg-config-library,SDL,sdl2))

ifeq ($(HAVE_WIN32),y)
# For Windows/SDL: Remove SDL's -Dmain=SDL_main macro which conflicts with
# XCSoar's code (e.g., PageLayout::main). We define SDL_MAIN_HANDLED instead
# and call SDL_SetMainReady() in XCSoar.cpp.
# Use lazy evaluation (=) to avoid triggering pkg-config errors at parse time
# Uses SDL_CPPFLAGS_RAW constructed through pkgconfig.mk/thunk.mk
SDL_CPPFLAGS = $(filter-out -Dmain=SDL_main,$(SDL_CPPFLAGS_RAW)) -DENABLE_SDL -DSDL_MAIN_HANDLED -DSDL_USE_BUILTIN_OPENGL_DEFINITIONS
else
SDL_CPPFLAGS += -DENABLE_SDL
endif

endif
