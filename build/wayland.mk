ifeq ($(USE_WAYLAND),y)
EGL = y
OPENGL = y

$(eval $(call pkg-config-library,WAYLAND,wayland-egl))
WAYLAND_FEATURE_CPPFLAGS = -DUSE_WAYLAND

# Generate C sources and headers from the Wayland protocol
# description; this is needed for interfaces which do not come
# pre-generated with libwayland

WAYLAND_GENERATED = $(TARGET_OUTPUT_DIR)/wayland-generated
INCLUDES += -isystem $(WAYLAND_GENERATED)

# from Debian package "wayland-protocols"
XDG_SHELL_XML = /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

# from Debian package "libwayland-bin"
WAYLAND_SCANNER = wayland-scanner

$(WAYLAND_GENERATED)/xdg-shell-client-protocol.h: $(XDG_SHELL_XML) | $(WAYLAND_GENERATED)/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(WAYLAND_SCANNER) client-header <$< >$@.tmp
	@mv $@.tmp $@

$(WAYLAND_GENERATED)/xdg-shell-public.c: $(XDG_SHELL_XML) | $(WAYLAND_GENERATED)/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(WAYLAND_SCANNER) public-code <$< >$@.tmp
	@mv $@.tmp $@

endif
