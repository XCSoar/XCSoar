/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "WaylandQueue.hpp"
#include "Queue.hpp"
#include "../Shared/Event.hpp"
#include "Util/StringAPI.hxx"

#include <wayland-client.h>

#include <stdio.h>
#include <stdlib.h>

static void
WaylandRegistryGlobal(void *data, struct wl_registry *registry, uint32_t id,
                      const char *interface, uint32_t version)
{
  auto &q = *(WaylandEventQueue *)data;
  q.RegistryHandler(registry, id, interface);
}

static void
WaylandRegistryGlobalRemove(void *data, struct wl_registry *registry,
                            uint32_t id)
{
}

static constexpr struct wl_registry_listener registry_listener = {
  WaylandRegistryGlobal,
  WaylandRegistryGlobalRemove,
};

static void
WaylandSeatHandleCapabilities(void *data, struct wl_seat *seat, uint32_t caps)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.SeatHandleCapabilities(caps & WL_SEAT_CAPABILITY_POINTER,
                               caps & WL_SEAT_CAPABILITY_KEYBOARD,
                               caps & WL_SEAT_CAPABILITY_TOUCH);
}

static constexpr struct wl_seat_listener seat_listener = {
  WaylandSeatHandleCapabilities,
};

static void
WaylandPointerEnter(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
                    struct wl_surface *surface,
                    wl_fixed_t surface_x,
                    wl_fixed_t surface_y)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerMotion(IntPoint2D(wl_fixed_to_int(surface_x),
                                 wl_fixed_to_int(surface_y)));
}

static void
WaylandPointerLeave(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
                    struct wl_surface *surface)
{
}

static void
WaylandPointerMotion(void *data, struct wl_pointer *wl_pointer, uint32_t time,
                     wl_fixed_t surface_x, wl_fixed_t surface_y)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerMotion(IntPoint2D(wl_fixed_to_int(surface_x),
                                 wl_fixed_to_int(surface_y)));
}

static void
WaylandPointerButton(void *data, struct wl_pointer *wl_pointer,
                     uint32_t serial, uint32_t time,
                     uint32_t button, uint32_t state)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerButton(state);
}

static void
WaylandPointerAxis(void *data, struct wl_pointer *wl_pointer,
                   uint32_t time, uint32_t axis, wl_fixed_t value)
{
  auto &queue = *(WaylandEventQueue *)data;

  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
    queue.Push(Event(Event::MOUSE_WHEEL, wl_fixed_to_int(value)));
}

static constexpr struct wl_pointer_listener pointer_listener = {
  WaylandPointerEnter,
  WaylandPointerLeave,
  WaylandPointerMotion,
  WaylandPointerButton,
  WaylandPointerAxis,
};

WaylandEventQueue::WaylandEventQueue(boost::asio::io_service &io_service,
                                     EventQueue &_queue)
  :queue(_queue),
   display(wl_display_connect(nullptr)), fd(io_service)
{
  if (display == nullptr) {
    fprintf(stderr, "wl_display_connect() failed\n");
    exit(EXIT_FAILURE);
  }

  auto registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener, this);

  wl_display_dispatch(display);
  wl_display_roundtrip(display);

  if (compositor == nullptr) {
    fprintf(stderr, "No Wayland compositor found\n");
    exit(EXIT_FAILURE);
  }

  if (seat == nullptr) {
    fprintf(stderr, "No Wayland seat found\n");
    exit(EXIT_FAILURE);
  }

  if (shell == nullptr) {
    fprintf(stderr, "No Wayland shell found\n");
    exit(EXIT_FAILURE);
  }

  fd.assign(wl_display_get_fd(display));
  AsyncRead();
}

WaylandEventQueue::~WaylandEventQueue()
{
  fd.cancel();
  wl_display_disconnect(display);
}

bool
WaylandEventQueue::Generate(Event &event)
{
  wl_display_flush(display);
  return false;
}

void
WaylandEventQueue::OnReadReady(const boost::system::error_code &ec)
{
  if (ec)
    return;

  wl_display_dispatch(display);
  AsyncRead();
}

inline void
WaylandEventQueue::RegistryHandler(struct wl_registry *registry, uint32_t id,
                                   const char *interface)
{
  if (StringIsEqual(interface, "wl_compositor"))
    compositor = (wl_compositor *)
      wl_registry_bind(registry, id, &wl_compositor_interface, 1);
  else if (StringIsEqual(interface, "wl_seat")) {
    seat = (wl_seat *)wl_registry_bind(registry, id,
                                         &wl_seat_interface, 1);
    wl_seat_add_listener(seat, &seat_listener, this);
  } else if (StringIsEqual(interface, "wl_shell"))
    shell = (wl_shell *)wl_registry_bind(registry, id,
                                         &wl_shell_interface, 1);
}

inline void
WaylandEventQueue::SeatHandleCapabilities(bool has_pointer, bool has_keyboard,
                                          bool has_touch)
{
  /* TODO: collect flags for HasTouchScreen(), HasPointer(),
     HasKeyboard(), HasCursorKeys() */

  if (has_pointer) {
    if (pointer == nullptr) {
      pointer = wl_seat_get_pointer(seat);
      if (pointer != nullptr)
        wl_pointer_add_listener(pointer, &pointer_listener, this);
    }
  } else {
    if (pointer != nullptr)
      wl_pointer_destroy(pointer);
  }

  // TODO: support keyboard devices
}

inline void
WaylandEventQueue::Push(const Event &event)
{
  queue.Push(event);
}

inline void
WaylandEventQueue::PointerMotion(IntPoint2D new_pointer_position)
{
  if (new_pointer_position == pointer_position)
    return;

  pointer_position = new_pointer_position;
  Push(Event(Event::MOUSE_MOTION,
             PixelPoint(pointer_position.x, pointer_position.y)));
}

inline void
WaylandEventQueue::PointerButton(bool pressed)
{
  Push(Event(pressed ? Event::MOUSE_DOWN : Event::MOUSE_UP,
             PixelPoint(pointer_position.x, pointer_position.y)));
}
