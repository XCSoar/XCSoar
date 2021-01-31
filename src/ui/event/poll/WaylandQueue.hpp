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

#ifndef XCSOAR_EVENT_X11_EVENT_QUEUE_HPP
#define XCSOAR_EVENT_X11_EVENT_QUEUE_HPP

#include "event/SocketEvent.hxx"
#include "Math/Point2D.hpp"

#include <cstdint>

struct wl_display;
struct wl_compositor;
struct wl_seat;
struct wl_pointer;
struct wl_shell;
struct wl_registry;

namespace UI {

class EventQueue;
struct Event;

/**
 * This class opens a connection to the X11 server using Xlib and
 * listens for events.
 */
class WaylandEventQueue final {
  EventQueue &queue;

  struct wl_display *const display;
  struct wl_compositor *compositor = nullptr;
  struct wl_seat *seat = nullptr;
  struct wl_pointer *pointer = nullptr;
  struct wl_shell *shell = nullptr;

  IntPoint2D pointer_position = {0, 0};

  SocketEvent socket_event;

public:
  /**
   * @param queue the #EventQueue that shall receive Wayland input
   * events
   */
  explicit WaylandEventQueue(EventQueue &queue);

  ~WaylandEventQueue();

  struct wl_display *GetDisplay() {
    return display;
  }

  struct wl_compositor *GetCompositor() {
    return compositor;
  }

  struct wl_shell *GetShell() {
    return shell;
  }

  bool IsVisible() const {
    // TODO: implement
    return true;
  }

  bool Generate(Event &event);

  void RegistryHandler(struct wl_registry *registry, uint32_t id,
                       const char *interface);

  void SeatHandleCapabilities(bool pointer, bool keyboard, bool touch);

  void Push(const Event &event);
  void PointerMotion(IntPoint2D new_pointer_position);
  void PointerButton(bool pressed);

private:
  void OnSocketReady(unsigned events) noexcept;
};

} // namespace UI

#endif
