// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"
#include "ui/event/shared/Event.hpp"
#include "ui/display/Display.hpp"
#include "ui/egl/System.hpp"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"

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
handle_configure([[maybe_unused]] void *data,
                 [[maybe_unused]] struct wl_shell_surface *shell_surface,
                 [[maybe_unused]] uint32_t edges,
                 int32_t width,
                 int32_t height) noexcept
{
  if (width > 0 && height > 0 && UI::event_queue != nullptr) {
    UI::event_queue->Inject(Event(Event::RESIZE,
                                  PixelPoint(width, height)));
  }
}

static void
handle_popup_done([[maybe_unused]] void *data,
                  [[maybe_unused]] struct wl_shell_surface *shell_surface) noexcept
{
}

static constexpr struct wl_shell_surface_listener shell_surface_listener = {
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
handle_surface_configure([[maybe_unused]] void *data,
                         struct xdg_surface *xdg_surface,
                         uint32_t serial) noexcept
{
  xdg_surface_ack_configure(xdg_surface, serial);
}

static constexpr struct xdg_surface_listener surface_listener = {
  .configure = handle_surface_configure,
};

static void
handle_toplevel_configure([[maybe_unused]] void *data,
                          [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
                          int32_t width,
                          int32_t height,
                          [[maybe_unused]] struct wl_array *states) noexcept
{
  if (width > 0 && height > 0 && UI::event_queue != nullptr) {
    UI::event_queue->Inject(Event(Event::RESIZE,
                                  PixelPoint(width, height)));
  }
}

static void
handle_toplevel_close([[maybe_unused]] void *data,
		      [[maybe_unused]] struct xdg_toplevel *xdg_toplevel) noexcept
{
  // TODO
}

static const struct xdg_toplevel_listener toplevel_listener = {
  .configure = handle_toplevel_configure,
  .close = handle_toplevel_close,
};

void
TopWindow::CreateNative(const TCHAR *text, PixelSize size,
                        TopWindowStyle)
{
  auto compositor = event_queue->GetCompositor();

  auto surface = wl_compositor_create_surface(compositor);
  if (surface == nullptr)
    throw std::runtime_error("Failed to create Wayland surface");

  if (auto wm_base = event_queue->GetWmBase()) {
    xdg_wm_base_add_listener(wm_base, &wm_base_listener, nullptr);

    const auto xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &surface_listener, nullptr);

    const auto toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(toplevel, &toplevel_listener, nullptr);
    xdg_toplevel_set_title(toplevel, text);
    xdg_toplevel_set_app_id(toplevel, "xcsoar");

    // TODO xdg_toplevel_set_fullscreen

    wl_surface_commit(surface);

    /* this roundtrip invokes handle_surface_configure() */
    wl_display_roundtrip(display.GetWaylandDisplay());

    // Request server-side decorations (window decorations with resize handles)
    // This must be done after the surface is committed and the window is mapped.
    // Note: Some compositors like Sway don't show traditional resize handles
    // even when server-side decorations are requested (by design).
    auto *decoration_mgr = event_queue->GetDecorationManager();
    if (decoration_mgr != nullptr) {
      auto *toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
        decoration_mgr, toplevel);
      if (toplevel_decoration != nullptr) {
        // Request server-side decorations (SSD) which include resize handles
        zxdg_toplevel_decoration_v1_set_mode(toplevel_decoration,
                                             ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        // Keep the decoration object alive - it will be destroyed when the toplevel is destroyed
        // Note: We could destroy it immediately after set_mode, but keeping it alive
        // allows the compositor to send mode change events if needed
      }
    }
  } else if (auto shell = event_queue->GetShell()) {
    auto shell_surface = wl_shell_get_shell_surface(shell, surface);
    wl_shell_surface_add_listener(shell_surface,
                                  &shell_surface_listener, nullptr);
    wl_shell_surface_set_toplevel(shell_surface);
    wl_shell_surface_set_title(shell_surface, text);

    // TODO: wl_shell_surface_set_fullscreen(shell_surface);
  }

  const auto region = wl_compositor_create_region(compositor);
  wl_region_add(region, 0, 0, size.width, size.height);
  wl_surface_set_opaque_region(surface, region);

  native_window = wl_egl_window_create(surface, size.width, size.height);
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
  // TODO: implement
}

void
TopWindow::DisableCapture() noexcept
{
  // TODO: implement
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  if (native_window != nullptr) {
    // Resize the EGL window to match the new size
    wl_egl_window_resize(native_window, new_size.width, new_size.height, 0, 0);

    // Update the opaque region
    auto compositor = event_queue->GetCompositor();
    if (compositor != nullptr) {
      const auto region = wl_compositor_create_region(compositor);
      wl_region_add(region, 0, 0, new_size.width, new_size.height);
      // Note: We can't update the surface's opaque region here because we
      // don't store the surface. The region will be updated on the next
      // commit, but this should be fine for most cases.
      wl_region_destroy(region);
    }
  }

  event_queue->SetScreenSize(new_size);
  ContainerWindow::OnResize(new_size);
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

  case Event::RESIZE:
    if (screen->CheckResize(PixelSize(event.point.x, event.point.y)))
      Resize(screen->GetSize());
    return true;
  }

  return false;
}

} // namespace UI
