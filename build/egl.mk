ifeq ($(TARGET_IS_PI),y)
# auto-enable EGL on the Raspberry Pi.
EGL ?= y
else ifeq ($(TARGET_HAS_MALI),y)
# auto-enable EGL on the Cubieboard.
EGL ?= y
else ifeq ($(ENABLE_MESA_KMS),y)
# if Mesa KMS is explicitly enabled, we also need to enable EGL
EGL ?= y
else ifneq ($(HAVE_WIN32)$(TARGET_IS_DARWIN)$(TARGET_IS_KOBO),nnn)
# Windows uses GDI
# Mac OS X and iOS use SDL
# Kobo uses software renderer on /dev/fb0
EGL = n
else ifeq ($(OPENGL),n)
# no EGL if OpenGL was disabled explicitly
EGL = n
else ifeq ($(ENABLE_SDL),y)
# no EGL if SDL was enabled explicitly
EGL = n
else ifeq ($(TARGET),ANDROID)
# Android uses Java-EGL
EGL ?= n
else ifeq ($(GLES),y)
# use EGL if GLES1 was chosen explicitly
EGL = y
else ifeq ($(GLES2),y)
# use EGL if GLES2 was chosen explicitly
EGL = y
else
# default to GLX/X11
EGL ?= n
endif

ifeq ($(EGL),y)

OPENGL = y

ifneq ($(TARGET),ANDROID)
FREETYPE = y
LIBPNG = y
LIBJPEG = y
endif

ENABLE_SDL = n

EGL_CPPFLAGS =
EGL_FEATURE_CPPFLAGS = -DUSE_EGL
EGL_LDLIBS = -lEGL

ifeq ($(TARGET_IS_PI),y)
# Raspberry Pi detected
EGL_FEATURE_CPPFLAGS += -DUSE_VIDEOCORE
EGL_CPPFLAGS += -isystem $(PI)/opt/vc/include -isystem $(PI)/opt/vc/include/interface/vcos/pthreads
EGL_CPPFLAGS += -isystem $(PI)/opt/vc/include/interface/vmcs_host/linux
EGL_LDLIBS += -L$(PI)/opt/vc/lib -lvchostif -lvchiq_arm -lvcos -lbcm_host
USE_CONSOLE = y
else ifeq ($(TARGET_HAS_MALI),y)
EGL_FEATURE_CPPFLAGS += -DHAVE_MALI
USE_CONSOLE = y
else ifeq ($(ENABLE_MESA_KMS),y)
$(eval $(call pkg-config-library,DRM,libdrm))
$(eval $(call pkg-config-library,GBM,gbm))
EGL_FEATURE_CPPFLAGS += -DMESA_KMS
EGL_CPPFLAGS += $(DRM_CPPFLAGS) $(GBM_CPPFLAGS)
EGL_LDLIBS += $(DRM_LDLIBS) $(GBM_LDLIBS)
USE_CONSOLE = y
else ifeq ($(USE_WAYLAND),y)
EGL_CPPFLAGS += $(WAYLAND_CPPFLAGS)
EGL_FEATURE_CPPFLAGS += $(WAYLAND_FEATURE_CPPFLAGS)
EGL_LDLIBS += $(WAYLAND_LDLIBS)
USE_CONSOLE = n
else ifeq ($(TARGET),ANDROID)
else
USE_X11 = y
EGL_FEATURE_CPPFLAGS += -DUSE_X11
EGL_LDLIBS += -lX11
USE_CONSOLE = n
endif

ifneq ($(TARGET),ANDROID)
USE_POLL_EVENT = y
endif

endif

USE_X11 ?= n
