// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"
#include "ui/event/shared/Event.hpp"
#include "ui/display/Display.hpp"
#include "Asset.hpp"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#ifdef SOFTWARE_ROTATE_DISPLAY
#include "DisplayOrientation.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#endif

#include <stdexcept>
#include <chrono>

namespace UI {

static void
handle_ping([[maybe_unused]] void *data,
            struct wl_shell_surface *shell_surface,
            uint32_t serial) noexcept
{
  wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data,
                 [[maybe_unused]] struct wl_shell_surface *shell_surface,
                 [[maybe_unused]] uint32_t edges,
                 int32_t width,
                 int32_t height) noexcept
{
  if (width > 0 && height > 0) {
    auto *window = static_cast<TopWindow *>(data);
    window->Resize(PixelSize(width, height));
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
handle_toplevel_configure(void *data,
                          [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
                          int32_t width,
                          int32_t height,
                          [[maybe_unused]] struct wl_array *states) noexcept
{
  auto *window = static_cast<TopWindow *>(data);
  
  /* In Wayland, width=0 or height=0 means "client decides" or "use current size".
   * When going fullscreen, the compositor may send 0x0 first (state change),
   * then send another configure with actual dimensions. We should always
   * process configure events immediately, even during resize drags, to provide
   * immediate visual feedback.
   * 
   * When dimensions are 0x0, the compositor will send another configure event
   * with actual dimensions. We don't need to handle 0x0 - just wait for the
   * next configure with real dimensions. The surface was already acked in
   * handle_surface_configure, and will be committed when we resize. */
  if (width > 0 && height > 0) {
    /* Compositor provided explicit dimensions - resize immediately.
     * This handles both normal resizes and fullscreen transitions where
     * the compositor sends actual screen dimensions. */
    window->OnNativeConfigure(PixelSize(width, height));
  }
  /* If width/height are 0, skip resize. The compositor will send another
   * configure event with actual dimensions, which we'll handle above. */
}

static void
handle_toplevel_close([[maybe_unused]] void *data,
                      [[maybe_unused]] struct xdg_toplevel *xdg_toplevel) noexcept
{
  if (event_queue != nullptr) {
    // Inject CLOSE event into the event queue
    event_queue->Inject(Event::CLOSE);
  }
}

static const struct xdg_toplevel_listener toplevel_listener = {
  .configure = handle_toplevel_configure,
  .close = handle_toplevel_close,
};

void
TopWindow::CreateNative(const char *text, PixelSize size,
                        TopWindowStyle style)
{
  /* Store initial requested size and reset configure flag */
  initial_requested_size = size;
  received_first_configure = false;
  last_resize_flush_time = std::chrono::steady_clock::now();
  
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
  if (native_window == nullptr)
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
TopWindow::OnNativeConfigure(PixelSize new_native_size) noexcept
{
  MarkFirstConfigureReceived();

  if (screen != nullptr) {
    screen->CheckResize(new_native_size);
    const PixelSize logical_size = screen->GetSize();
    if (logical_size == GetSize())
      BumpRenderStateToken();

    Resize(logical_size);
  } else
    Resize(new_native_size);
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  /* Skip initial resize from ContainerWindow::Create if we haven't received
   * the first configure event yet. This prevents the window from being
   * resized to the requested size before the compositor sends actual dimensions. */
  if (!received_first_configure && 
      new_size.width == initial_requested_size.width &&
      new_size.height == initial_requested_size.height) {
    return;
  }

  BumpRenderStateToken();

  PixelSize native_size = new_size;
#ifdef SOFTWARE_ROTATE_DISPLAY
  if (AreAxesSwapped(OpenGL::display_orientation))
    native_size = PixelSize(new_size.height, new_size.width);
#endif

  /* Use native size for input coordinate transformation. */
  event_queue->SetScreenSize(native_size);

  if (native_window != nullptr) {
    /* Update EGL window size */
    wl_egl_window_resize(native_window,
                         native_size.width, native_size.height, 0, 0);
  }

  /* Update opaque region for the new size BEFORE drawing.
   * This ensures the compositor knows the new size before we present a frame. */
  if (wl_surface != nullptr && event_queue != nullptr) {
    auto compositor = event_queue->GetCompositor();
    if (compositor != nullptr) {
      const auto region = wl_compositor_create_region(compositor);
      wl_region_add(region, 0, 0, native_size.width, native_size.height);
      wl_surface_set_opaque_region(wl_surface, region);
      wl_region_destroy(region);
      /* Don't commit yet - eglSwapBuffers() will commit automatically.
       * Committing here might cause the compositor to process the new size
       * before we've drawn the frame. */
    }
  }

  /* The native configure callback has already updated the GL viewport size.
     Orientation-only changes only need redraw/commit here. */
  if (screen != nullptr) {
    Invalidate();
    /* Force immediate redraw for visual feedback during resize.
     * However, throttle flush/dispatch during rapid resize drags to avoid
     * performance issues. Use longer throttle interval for e-paper displays
     * which have much lower refresh rates (1-2fps full, ~10-15fps partial). */
    if (screen->IsReady() && IsVisible()) {
      Expose();
      const auto now = std::chrono::steady_clock::now();
      const auto time_since_last_flush =
        std::chrono::duration_cast<std::chrono::milliseconds>(
          now - last_resize_flush_time).count();
      /* Use adaptive throttle based on display type:
       * - E-paper: 100ms (~10fps max) to match slow refresh rate
       * - Normal displays: 16ms (~60fps max) for smooth resizing */
      const int throttle_ms = HasEPaper() ? 100 : 16;
      if (time_since_last_flush >= throttle_ms) {
        struct wl_display *wl_display = display.GetWaylandDisplay();
        wl_display_flush(wl_display);
        wl_display_dispatch_pending(wl_display);
        last_resize_flush_time = now;
      }
    } else {
      /* If we can't draw now, commit the surface configuration so the
       * compositor knows about the new size. We'll draw later. */
      if (wl_surface != nullptr)
        wl_surface_commit(wl_surface);
    }
  }

  /* Call base implementation */
  ContainerWindow::OnResize(new_size);

#ifdef USE_MEMORY_CANVAS
  // Request resize instead of doing it immediately
  // The actual resize will happen in the draw thread (Expose)
  if (screen != nullptr)
    screen->RequestResize(new_size);
#endif
}


bool
TopWindow::OnEvent(const Event &event)
{
  switch (event.type) {
    Window *w;

  case Event::NOP:
  case Event::CALLBACK:
    break;

  case Event::CLOSE:
    OnClose();
    break;

  case Event::KEY_DOWN:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    return w->OnKeyDown(event.param);

  case Event::KEY_UP:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    return w->OnKeyUp(event.param);

  case Event::MOUSE_MOTION:
#ifdef DRAW_MOUSE_CURSOR
    cursor_visible_until = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    /* redraw to update the mouse cursor position */
    Invalidate();
#endif

    // XXX keys
    return OnMouseMove(event.point, 0);

  case Event::MOUSE_DOWN:
    return double_click.Check(event.point)
      ? OnMouseDouble(event.point)
      : OnMouseDown(event.point);

  case Event::MOUSE_UP:
    double_click.Moved(event.point);

    return OnMouseUp(event.point);

  case Event::MOUSE_WHEEL:
    return OnMouseWheel(event.point, (int)event.param);

#if defined(USE_X11) || defined(MESA_KMS)
  case Event::EXPOSE:
    Invalidate();
    return true;
#endif
  }

  return false;
}

} // namespace UI
