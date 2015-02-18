ifeq ($(USE_WAYLAND),y)
EGL = y
OPENGL = y

$(eval $(call pkg-config-library,WAYLAND,wayland-client))
WAYLAND_CPPFLAGS += -DUSE_WAYLAND
WAYLAND_LDLIBS += -lwayland-egl

endif
