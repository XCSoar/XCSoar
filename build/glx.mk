ifneq ($(HAVE_WIN32)$(TARGET_IS_DARWIN)$(TARGET_IS_ANDROID)$(TARGET_IS_KOBO),nnnn)
# Windows uses GDI
# Mac OS X and iOS use SDL
# Android uses Java-EGL
# Kobo uses software renderer on /dev/fb0
GLX = n
else ifeq ($(OPENGL),n)
# no GLX if OpenGL was disabled explicitly
GLX = n
else ifeq ($(ENABLE_SDL),y)
# no GLX if SDL was enabled explicitly
GLX = n
else ifneq ($(EGL),y)
# default to GLX/X11
GLX ?= y
endif

ifeq ($(GLX),y)
OPENGL = y
FREETYPE = y
LIBPNG = y
LIBJPEG = y
USE_X11 = y
ENABLE_SDL = n
USE_CONSOLE = n
USE_POLL_EVENT = y
GLX_CPPFLAGS = -DUSE_GLX -DUSE_X11
GLX_LDLIBS = -lX11
endif

USE_X11 ?= n
