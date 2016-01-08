ifeq ($(USE_WAYLAND),y)
EGL = y
OPENGL = y

$(eval $(call pkg-config-library,WAYLAND,wayland-egl))
WAYLAND_FEATURE_CPPFLAGS = -DUSE_WAYLAND

endif
