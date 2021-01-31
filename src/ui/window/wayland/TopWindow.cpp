/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "../TopWindow.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"

#include <stdexcept>

namespace UI {

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
            uint32_t serial) noexcept
{
  wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                 uint32_t edges, int32_t width, int32_t height) noexcept
{
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface) noexcept
{
}

static constexpr struct wl_shell_surface_listener shell_surface_listener = {
  handle_ping,
  handle_configure,
  handle_popup_done
};

void
TopWindow::CreateNative(const TCHAR *text, PixelSize size,
                        TopWindowStyle style)
{
  auto display = event_queue->GetDisplay();
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

  native_display = display;
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
