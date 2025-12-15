// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"
#include "ui/event/shared/Event.hpp"
#include "ui/display/Display.hpp"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"

#include <stdexcept>

namespace UI {

static void
handle_ping([[maybe_unused]] void *data,
            struct wl_shell_surface *shell_surface,
            uint32_t serial) noexcept
{
  wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure([[maybe_unused]] void *data,
                 [[maybe_unused]] struct wl_shell_surface *shell_surface,
                 [[maybe_unused]] uint32_t edges,
                 int32_t width,
                 int32_t height) noexcept
{
  if (width > 0 && height > 0 && event_queue != nullptr) {
    /* Inject RESIZE event into the event queue */
    event_queue->Inject(Event(Event::RESIZE,
                              PixelPoint(width, height)));
  }
}

static void
handle_popup_done([[maybe_unused]] void *data,
                  [[maybe_unused]] struct wl_shell_surface *shell_surface) noexcept
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
  .ping = handle_ping,
  .configure = handle_configure,
  .popup_done = handle_popup_done
};

static void
handle_wm_base_ping([[maybe_unused]] void *data,
                    struct xdg_wm_base *xdg_wm_base,
                    uint32_t serial) noexcept
{
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static constexpr struct xdg_wm_base_listener wm_base_listener = {
  .ping = handle_wm_base_ping,
};

static void
handle_surface_configure(void *data,
                         struct xdg_surface *xdg_surface,
                         uint32_t serial) noexcept
{
  auto *window = static_cast<TopWindow *>(data);
  xdg_surface_ack_configure(xdg_surface, serial);

  /* The actual size will be provided by handle_toplevel_configure.
     This callback just acknowledges the configure event */
  (void)window;
}

static const struct xdg_surface_listener surface_listener = {
  .configure = handle_surface_configure,
};

static void
handle_toplevel_configure([[maybe_unused]] void *data,
                          [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
                          int32_t width,
                          int32_t height,
                          [[maybe_unused]] struct wl_array *states) noexcept
{
  if (width > 0 && height > 0 && event_queue != nullptr) {
    /* Inject RESIZE event into the event queue */
    event_queue->Inject(Event(Event::RESIZE,
                              PixelPoint(width, height)));
  }
}

static void
handle_toplevel_close([[maybe_unused]] void *data,
                      [[maybe_unused]] struct xdg_toplevel *xdg_toplevel) noexcept
{
  if (event_queue != nullptr) {
    /* Inject CLOSE event into the event queue */
    event_queue->Inject(Event::CLOSE);
  }
}

static const struct xdg_toplevel_listener toplevel_listener = {
  .configure = handle_toplevel_configure,
  .close = handle_toplevel_close,
};

void
TopWindow::CreateNative(const TCHAR *text, PixelSize size,
                        TopWindowStyle style)
{
  auto compositor = event_queue->GetCompositor();

  wl_surface = wl_compositor_create_surface(compositor);
  if (wl_surface == nullptr)
    throw std::runtime_error("Failed to create Wayland surface");

  if (auto wm_base = event_queue->GetWmBase()) {
    xdg_wm_base_add_listener(wm_base, &wm_base_listener, nullptr);

    xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, wl_surface);
    xdg_surface_add_listener(xdg_surface, &surface_listener, this);

    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &toplevel_listener, this);
    xdg_toplevel_set_title(xdg_toplevel, text);
    xdg_toplevel_set_app_id(xdg_toplevel, "xcsoar");

    /* Request server-side decorations (titlebar) if available */
    if (auto decoration_manager = event_queue->GetDecorationManager()) {
      xdg_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
        decoration_manager, xdg_toplevel);
      if (xdg_decoration != nullptr) {
        /* Request server-side decorations (titlebar provided by
           compositor) */
        zxdg_toplevel_decoration_v1_set_mode(xdg_decoration,
          ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
      }
    }

    if (style.GetFullScreen()) {
      /* Request fullscreen on default output (NULL = default) */
      xdg_toplevel_set_fullscreen(xdg_toplevel, nullptr);
    }

    wl_surface_commit(wl_surface);

    /* this roundtrip invokes handle_surface_configure() */
    wl_display_roundtrip(display.GetWaylandDisplay());
  } else if (auto shell = event_queue->GetShell()) {
    auto shell_surface = wl_shell_get_shell_surface(shell, wl_surface);
    wl_shell_surface_add_listener(shell_surface,
                                  &shell_surface_listener, this);
    wl_shell_surface_set_toplevel(shell_surface);
    wl_shell_surface_set_title(shell_surface, text);

    if (style.GetFullScreen()) {
      /* Request fullscreen on default output (NULL = default).
         WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT = 0 */
      wl_shell_surface_set_fullscreen(shell_surface,
                                      WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                      0, nullptr);
    }
  }

  const auto region = wl_compositor_create_region(compositor);
  wl_region_add(region, 0, 0, size.width, size.height);
  wl_surface_set_opaque_region(wl_surface, region);
  wl_region_destroy(region);

  native_window = wl_egl_window_create(wl_surface, size.width, size.height);
  if (native_window == EGL_NO_SURFACE)
    throw std::runtime_error("Failed to create Wayland EGL window");
}

bool
TopWindow::IsVisible() const noexcept
{
  return event_queue->IsVisible();
}

void
TopWindow::EnableCapture() noexcept
{
  /* In Wayland, pointer events naturally go to the surface that has
     focus. For full pointer locking/confining (equivalent to X11's
     XGrabPointer), we would need the zwp_pointer_constraints_v1
     extension protocol. For now, we ensure the pointer is available -
     the compositor handles pointer focus to our surface automatically.

     Note: Full pointer locking would require:
     - Binding zwp_pointer_constraints_v1 from registry
     - Creating zwp_locked_pointer_v1 or zwp_confined_pointer_v1
     This is a placeholder for future enhancement with pointer
     constraints. */
  (void)event_queue;
  (void)wl_surface;
}

void
TopWindow::DisableCapture() noexcept
{
  /* In Wayland, pointer capture is automatically released when the
     surface loses focus. If we implemented pointer constraints, we
     would destroy the locked_pointer or confined_pointer object here. */
  (void)this;
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  if (native_window != nullptr) {
    /* Update EGL window size */
    wl_egl_window_resize(native_window, new_size.width, new_size.height, 0, 0);
  }

  /* Update opaque region for the new size */
  if (wl_surface != nullptr && event_queue != nullptr) {
    auto compositor = event_queue->GetCompositor();
    if (compositor != nullptr) {
      const auto region = wl_compositor_create_region(compositor);
      wl_region_add(region, 0, 0, new_size.width, new_size.height);
      wl_surface_set_opaque_region(wl_surface, region);
      wl_region_destroy(region);
      wl_surface_commit(wl_surface);
    }
  }

  /* Call base implementation */
  ContainerWindow::OnResize(new_size);

#ifdef USE_MEMORY_CANVAS
  screen->OnResize(new_size);
#endif
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  /* Update event queue screen size (required for proper coordinate transformation) */
  event_queue->SetScreenSize(new_size);

  if (native_window != nullptr) {
    /* Update EGL window size */
    wl_egl_window_resize(native_window, new_size.width, new_size.height, 0, 0);
  }

  /* Check if screen needs to be resized and update OpenGL viewport */
  if (screen != nullptr && screen->CheckResize(new_size)) {
    /* Screen was resized, trigger redraw */
    Invalidate();
  }

  /* Update opaque region for the new size */
  if (wl_surface != nullptr && event_queue != nullptr) {
    auto compositor = event_queue->GetCompositor();
    if (compositor != nullptr) {
      const auto region = wl_compositor_create_region(compositor);
      wl_region_add(region, 0, 0, new_size.width, new_size.height);
      wl_surface_set_opaque_region(wl_surface, region);
      wl_region_destroy(region);
      wl_surface_commit(wl_surface);
    }
  }

  /* Call base implementation */
  ContainerWindow::OnResize(new_size);

#ifdef USE_MEMORY_CANVAS
  screen->OnResize(new_size);
#endif
}

} // namespace UI
