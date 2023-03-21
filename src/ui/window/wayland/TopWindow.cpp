// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"

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
  handle_ping,
  handle_configure,
  handle_popup_done
};

void
TopWindow::CreateNative(const TCHAR *text, PixelSize size,
                        TopWindowStyle)
{
  auto compositor = event_queue->GetCompositor();
  auto shell = event_queue->GetShell();

  auto surface = wl_compositor_create_surface(compositor);
  if (surface == nullptr)
    throw std::runtime_error("Failed to create Wayland surface");

  auto shell_surface = wl_shell_get_shell_surface(shell, surface);
  wl_shell_surface_add_listener(shell_surface,
                                &shell_surface_listener, nullptr);
  wl_shell_surface_set_toplevel(shell_surface);
  wl_shell_surface_set_title(shell_surface, text);

  // TODO: wl_shell_surface_set_fullscreen(shell_surface);

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
