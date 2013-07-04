ifeq ($(TARGET)$(TARGET_IS_PI),UNIXy)
# auto-enable EGL on the Raspberry Pi.
EGL ?= y
else ifeq ($(TARGET)$(TARGET_HAS_MALI),UNIXy)
# auto-enable EGL on the Cubieboard.
EGL ?= y
else
EGL ?= n
endif

ifeq ($(EGL),y)

OPENGL = y
FREETYPE = y
LIBPNG = y
LIBJPEG = y
ENABLE_SDL = n

EGL_CPPFLAGS = -DUSE_EGL
EGL_LDLIBS = -lEGL

ifeq ($(TARGET_IS_PI),y)
# Raspberry Pi detected
EGL_CPPFLAGS += -DUSE_VIDEOCORE
EGL_CPPFLAGS += -isystem $(PI)/opt/vc/include -isystem $(PI)/opt/vc/include/interface/vcos/pthreads
EGL_CPPFLAGS += -isystem $(PI)/opt/vc/include/interface/vmcs_host/linux
EGL_LDLIBS += -L$(PI)/opt/vc/lib -lvchostif -lvchiq_arm -lvcos -lbcm_host
else ifeq ($(TARGET_HAS_MALI),y)
EGL_CPPFLAGS += -DHAVE_MALI
else
EGL_CPPFLAGS += -DUSE_X11
EGL_LDLIBS += -lX11
endif

endif
