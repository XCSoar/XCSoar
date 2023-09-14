// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaylandQueue.hpp"
#include "Queue.hpp"
#include "../shared/Event.hpp"
#include "ui/display/Display.hpp"
#include "util/StringAPI.hxx"
#include "xdg-shell-client-protocol.h"

#include <wayland-client.h>

#include <cerrno>
#include <cstdio> // for fprintf()
#include <cstdlib> // for abort()
#include <cstring> // for strerror()
#include <stdexcept>

namespace UI {

static void
WaylandRegistryGlobal(void *data, struct wl_registry *registry, uint32_t id,
                      const char *interface, [[maybe_unused]] uint32_t version)
{
  auto &q = *(WaylandEventQueue *)data;
  q.RegistryHandler(registry, id, interface);
}

static void
WaylandRegistryGlobalRemove([[maybe_unused]] void *data,
                            [[maybe_unused]] struct wl_registry *registry,
                            [[maybe_unused]] uint32_t id)
{
}

static constexpr struct wl_registry_listener registry_listener = {
  .global = WaylandRegistryGlobal,
  .global_remove = WaylandRegistryGlobalRemove,
};

static void
WaylandSeatHandleCapabilities(void *data,
                              [[maybe_unused]] struct wl_seat *seat,
                              uint32_t caps)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.SeatHandleCapabilities(caps & WL_SEAT_CAPABILITY_POINTER,
                               caps & WL_SEAT_CAPABILITY_KEYBOARD,
                               caps & WL_SEAT_CAPABILITY_TOUCH);
}

static constexpr struct wl_seat_listener seat_listener = {
  .capabilities = WaylandSeatHandleCapabilities,
};

static void
WaylandPointerEnter(void *data,
                    [[maybe_unused]] struct wl_pointer *wl_pointer,
                    [[maybe_unused]] uint32_t serial,
                    [[maybe_unused]] struct wl_surface *surface,
                    wl_fixed_t surface_x,
                    wl_fixed_t surface_y)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerMotion(IntPoint2D(wl_fixed_to_int(surface_x),
                                 wl_fixed_to_int(surface_y)));
}

static void
WaylandPointerLeave([[maybe_unused]] void *data,
                    [[maybe_unused]] struct wl_pointer *wl_pointer,
                    [[maybe_unused]] uint32_t serial,
                    [[maybe_unused]] struct wl_surface *surface)
{
}

static void
WaylandPointerMotion(void *data,
                     [[maybe_unused]] struct wl_pointer *wl_pointer,
                     [[maybe_unused]] uint32_t time,
                     wl_fixed_t surface_x, wl_fixed_t surface_y)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerMotion(IntPoint2D(wl_fixed_to_int(surface_x),
                                 wl_fixed_to_int(surface_y)));
}

static void
WaylandPointerButton(void *data,
                     [[maybe_unused]] struct wl_pointer *wl_pointer,
                     [[maybe_unused]] uint32_t serial,
                     [[maybe_unused]] uint32_t time,
                     [[maybe_unused]] uint32_t button,
                     uint32_t state)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerButton(state);
}

static void
WaylandPointerAxis(void *data,
                   [[maybe_unused]] struct wl_pointer *wl_pointer,
                   [[maybe_unused]] uint32_t time,
                   uint32_t axis, wl_fixed_t value)
{
  auto &queue = *(WaylandEventQueue *)data;

  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
    queue.Push(Event(Event::MOUSE_WHEEL, wl_fixed_to_int(value)));
}

static constexpr struct wl_pointer_listener pointer_listener = {
  .enter = WaylandPointerEnter,
  .leave = WaylandPointerLeave,
  .motion = WaylandPointerMotion,
  .button = WaylandPointerButton,
  .axis = WaylandPointerAxis,
};

static void
WaylandKeyboardKeymap([[maybe_unused]] void *data,
                      [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                      [[maybe_unused]] uint32_t format,
                      [[maybe_unused]] int32_t fd,
                      [[maybe_unused]] uint32_t size) noexcept
{
}

static void
WaylandKeyboardEnter([[maybe_unused]] void *data,
                     [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                     [[maybe_unused]] uint32_t serial,
                     [[maybe_unused]] struct wl_surface *surface,
                     [[maybe_unused]] struct wl_array *keys) noexcept
{
}

static void
WaylandKeyboardLeave([[maybe_unused]] void *data,
                     [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                     [[maybe_unused]] uint32_t serial,
                     [[maybe_unused]] struct wl_surface *surface) noexcept
{
}

static void
WaylandKeyboardKey(void *data,
                   [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                   [[maybe_unused]] uint32_t serial,
                   [[maybe_unused]] uint32_t time,
                   uint32_t key,
                   uint32_t state) noexcept
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.KeyboardKey(key, state);
}

static void
WaylandKeyboardModifiers([[maybe_unused]] void *data,
                         [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                         [[maybe_unused]] uint32_t serial,
                         [[maybe_unused]] uint32_t mods_depressed,
                         [[maybe_unused]] uint32_t mods_latched,
                         [[maybe_unused]] uint32_t mods_locked,
                         [[maybe_unused]] uint32_t group) noexcept
{
}

static void
WaylandKeyboardRepeatInfo([[maybe_unused]] void *data,
                          [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                          [[maybe_unused]] int32_t rate,
                          [[maybe_unused]] int32_t delay) noexcept
{
}

static constexpr struct wl_keyboard_listener keyboard_listener = {
  .keymap = WaylandKeyboardKeymap,
  .enter = WaylandKeyboardEnter,
  .leave = WaylandKeyboardLeave,
  .key = WaylandKeyboardKey,
  .modifiers = WaylandKeyboardModifiers,
  .repeat_info = WaylandKeyboardRepeatInfo,
};

WaylandEventQueue::WaylandEventQueue(UI::Display &_display, EventQueue &_queue)
  :queue(_queue),
   display(_display.GetWaylandDisplay()),
   socket_event(queue.GetEventLoop(), BIND_THIS_METHOD(OnSocketReady)),
   flush_event(queue.GetEventLoop(), BIND_THIS_METHOD(OnFlush))
{
  if (display == nullptr)
    throw std::runtime_error("wl_display_connect() failed");

  auto registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener, this);

  wl_display_dispatch(display);
  wl_display_roundtrip(display);

  if (compositor == nullptr)
    throw std::runtime_error("No Wayland compositor found");

  if (seat == nullptr)
    throw std::runtime_error("No Wayland seat found");

  if (wm_base == nullptr && shell == nullptr)
    throw std::runtime_error{"No Wayland xdg_wm_base/shell found"};

  socket_event.Open(SocketDescriptor(wl_display_get_fd(display)));
  socket_event.ScheduleRead();
  flush_event.Schedule();
}

bool
WaylandEventQueue::Generate([[maybe_unused]] Event &event) noexcept
{
  flush_event.Schedule();
  return false;
}

void
WaylandEventQueue::OnSocketReady(unsigned events) noexcept
{
  flush_event.Schedule();

  if (wl_display_dispatch(display) < 0) {
    const int e = errno;
    if (e != EAGAIN) {
      fprintf(stderr, "wl_display_dispatch() failed: %s\n", strerror(e));
      abort();
    }
  }

  if (events & SocketEvent::HANGUP) {
    fprintf(stderr, "Wayland server hung up\n");
    abort();
  }
}

void
WaylandEventQueue::OnFlush() noexcept
{
  if (wl_display_flush(display) < 0) {
    const int e = errno;
    if (e != EAGAIN) {
      fprintf(stderr, "wl_display_flush() failed: %s\n", strerror(e));
      abort();
    }
  }
}

inline void
WaylandEventQueue::RegistryHandler(struct wl_registry *registry, uint32_t id,
                                   const char *interface) noexcept
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
  else if (StringIsEqual(interface, "xdg_wm_base"))
    wm_base = (xdg_wm_base *)wl_registry_bind(registry, id,
                                              &xdg_wm_base_interface, 1);
}

inline void
WaylandEventQueue::SeatHandleCapabilities(bool has_pointer, bool has_keyboard,
                                          bool has_touch) noexcept
{
  /* TODO: collect flags for HasCursorKeys() */

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

  if (has_keyboard) {
    if (keyboard == nullptr) {
      keyboard = wl_seat_get_keyboard(seat);
      if (keyboard != nullptr)
        wl_keyboard_add_listener(keyboard, &keyboard_listener, this);
    }
  } else {
    if (keyboard != nullptr)
      wl_keyboard_destroy(keyboard);
  }

  has_touchscreen = has_touch;
}

inline void
WaylandEventQueue::Push(const Event &event) noexcept
{
  queue.Push(event);
}

inline void
WaylandEventQueue::PointerMotion(IntPoint2D new_pointer_position) noexcept
{
  if (new_pointer_position == pointer_position)
    return;

  pointer_position = new_pointer_position;
  Push(Event(Event::MOUSE_MOTION,
             PixelPoint(pointer_position.x, pointer_position.y)));
}

inline void
WaylandEventQueue::PointerButton(bool pressed) noexcept
{
  Push(Event(pressed ? Event::MOUSE_DOWN : Event::MOUSE_UP,
             PixelPoint(pointer_position.x, pointer_position.y)));
}

void
WaylandEventQueue::KeyboardKey(uint32_t key, uint32_t state) noexcept
{
  switch (state) {
  case WL_KEYBOARD_KEY_STATE_RELEASED:
    queue.Push(Event{Event::KEY_UP, key});
    break;

  case WL_KEYBOARD_KEY_STATE_PRESSED:
    queue.Push(Event{Event::KEY_DOWN, key});
    break;
  }
}

} // namespace UI
