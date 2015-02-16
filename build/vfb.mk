# the Virtual Frame Buffer, a non-interactive Screen implementation
# (for debugging and profiling)

VFB ?= n

ifeq ($(VFB),y)
USE_FB = n
USE_POLL_EVENT = y
USE_CONSOLE = n
USE_MEMORY_CANVAS = y
FREETYPE = y
LIBPNG = y
LIBJPEG = y
EGL = n
OPENGL = n
ENABLE_SDL = n
endif
