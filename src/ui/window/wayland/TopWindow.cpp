// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"
#include "ui/display/Display.hpp"
#include "xdg-shell-client-protocol.h"

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
                 [[maybe_unused]] int32_t width,
                 [[maybe_unused]] int32_t height) noexcept
{
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
                          [[maybe_unused]] int32_t width,
                          [[maybe_unused]] int32_t height,
                          [[maybe_unused]] struct wl_array *states) noexcept
{
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

} // namespace UI
