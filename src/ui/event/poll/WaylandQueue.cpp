// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef ENABLE_OPENGL
#include "ui/opengl/Features.hpp"
#endif
#include "../shared/Event.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "Queue.hpp"
#include "WaylandQueue.hpp"
#include "ui/display/Display.hpp"
#include "util/EnvParser.hpp"
#include "util/StringAPI.hxx"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "../shared/TransformCoordinates.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/dim/Size.hpp"
#endif

#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>

#include <cerrno>
#include <cstdio>  // for fprintf()
#include <cstdlib> // for abort(), getenv()
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
WaylandPointerEnter(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
                    [[maybe_unused]] struct ::wl_surface *surface,
                    wl_fixed_t surface_x, wl_fixed_t surface_y)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerMotion(
      IntPoint2D(wl_fixed_to_int(surface_x), wl_fixed_to_int(surface_y)));

  /* Set cursor to pointer when entering the surface */
  queue.SetCursor(wl_pointer, serial);
}

static void
WaylandPointerLeave([[maybe_unused]] void *data,
                    [[maybe_unused]] struct wl_pointer *wl_pointer,
                    [[maybe_unused]] uint32_t serial,
                    [[maybe_unused]] struct ::wl_surface *surface)
{
}

static void
WaylandPointerMotion(void *data,
                     [[maybe_unused]] struct wl_pointer *wl_pointer,
                     [[maybe_unused]] uint32_t time, wl_fixed_t surface_x,
                     wl_fixed_t surface_y)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerMotion(
      IntPoint2D(wl_fixed_to_int(surface_x), wl_fixed_to_int(surface_y)));
}

static void
WaylandPointerButton(void *data,
                     [[maybe_unused]] struct wl_pointer *wl_pointer,
                     [[maybe_unused]] uint32_t serial,
                     [[maybe_unused]] uint32_t time,
                     [[maybe_unused]] uint32_t button, uint32_t state)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerButton(state);
}

static void
WaylandPointerAxis(void *data, [[maybe_unused]] struct wl_pointer *wl_pointer,
                   [[maybe_unused]] uint32_t time, uint32_t axis,
                   wl_fixed_t value)
{
  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    auto &q = *(WaylandEventQueue *)data;
#ifdef SOFTWARE_ROTATE_DISPLAY
    PixelPoint p = q.GetTransformedPointerPosition();
#else
    PixelPoint p(q.pointer_position.x, q.pointer_position.y);
#endif
    Event e(Event::MOUSE_WHEEL, p);
    /* Invert scroll direction to match X11 convention (positive = scroll up) */
    e.param = -wl_fixed_to_int(value);
    q.Push(e);
  }
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
                     [[maybe_unused]] struct ::wl_surface *surface,
                     [[maybe_unused]] struct wl_array *keys) noexcept
{
}

static void
WaylandKeyboardLeave([[maybe_unused]] void *data,
                     [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                     [[maybe_unused]] uint32_t serial,
                     [[maybe_unused]] struct ::wl_surface *surface) noexcept
{
}

static void
WaylandKeyboardKey(void *data,
                   [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                   [[maybe_unused]] uint32_t serial,
                   [[maybe_unused]] uint32_t time, uint32_t key,
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
    : queue(_queue), display(_display.GetWaylandDisplay()),
      socket_event(queue.GetEventLoop(), BIND_THIS_METHOD(OnSocketReady)),
      flush_event(queue.GetEventLoop(), BIND_THIS_METHOD(OnFlush))
{
  if (display == nullptr)
    throw std::runtime_error("wl_display_connect() failed");

  auto registry = ::wl_display_get_registry(display);
  ::wl_registry_add_listener(registry, &registry_listener, this);

  ::wl_display_dispatch(display);
  ::wl_display_roundtrip(display);

  if (compositor == nullptr)
    throw std::runtime_error("No Wayland compositor found");

  if (seat == nullptr) throw std::runtime_error("No Wayland seat found");

  if (wm_base == nullptr && shell == nullptr)
    throw std::runtime_error{"No Wayland xdg_wm_base/shell found"};

  if (shm == nullptr) throw std::runtime_error("No Wayland wl_shm found");

  /* Load cursor theme. Use nullptr for theme name to use the default system
     theme. Cursor size: read from XCURSOR_SIZE environment variable
     (standard), or calculate from display DPI if available. Defaults to 24
     pixels for 96 DPI displays, scaling proportionally for higher DPI. */
  int cursor_size;
  /* Allow up to 2048 pixels to support very high-DPI displays (e.g., 3000 DPI
     -> ~750px). Cursor themes typically have sizes up to 256px, but we allow
     larger requests; the theme will use the closest available size. */
  cursor_size = GetEnvInt("XCURSOR_SIZE", 24, 1, 2048);
  if (cursor_size == 24) {
    /* If XCURSOR_SIZE not set or invalid, scale default cursor size based on DPI
       Default is 24px at 96 DPI (1/4 inch). Scale proportionally for
       higher DPI displays (e.g., 650 DPI -> ~162px, 3000 DPI -> ~750px). */
    const auto dpi = ::Display::GetDPI(_display, 0);
    if (dpi.x > 0) {
      /* Scale: cursor_size = 24 * (dpi / 96) */
      const int scaled_size = (24 * dpi.x + 48) / 96; /* +48 for rounding */
      if (scaled_size > 0 && scaled_size <= 2048) {
        cursor_size = scaled_size;
      }
    }
  }
  cursor_theme = ::wl_cursor_theme_load(nullptr, cursor_size, shm);
  if (cursor_theme != nullptr) {
    cursor_pointer = ::wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
    if (cursor_pointer == nullptr) {
      /* Fallback to default cursor name */
      cursor_pointer = ::wl_cursor_theme_get_cursor(cursor_theme, "default");
    }
    if (cursor_pointer != nullptr && compositor != nullptr) {
      cursor_surface = ::wl_compositor_create_surface(compositor);
    }
  }

  socket_event.Open(SocketDescriptor(::wl_display_get_fd(display)));
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

  if (::wl_display_dispatch(display) < 0) {
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
  if (::wl_display_flush(display) < 0) {
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
    compositor = (::wl_compositor *)::wl_registry_bind(
        registry, id, &::wl_compositor_interface, 1);
  else if (StringIsEqual(interface, "wl_seat")) {
    seat = (::wl_seat *)::wl_registry_bind(registry, id, &::wl_seat_interface, 1);
    ::wl_seat_add_listener(seat, &seat_listener, this);
  } else if (StringIsEqual(interface, "wl_shell"))
    shell = (::wl_shell *)::wl_registry_bind(registry, id, &::wl_shell_interface, 1);
  else if (StringIsEqual(interface, "xdg_wm_base"))
    wm_base = (::xdg_wm_base *)::wl_registry_bind(registry, id,
                                              &::xdg_wm_base_interface, 1);
  else if (StringIsEqual(interface, "zxdg_decoration_manager_v1"))
    decoration_manager = (::zxdg_decoration_manager_v1 *)
      ::wl_registry_bind(registry, id, &::zxdg_decoration_manager_v1_interface, 1);
  else if (StringIsEqual(interface, "wl_shm"))
    shm = (::wl_shm *)::wl_registry_bind(registry, id, &::wl_shm_interface, 1);
}

inline void
WaylandEventQueue::SeatHandleCapabilities(bool has_pointer, bool has_keyboard,
                                          bool has_touch) noexcept
{
  /* TODO: collect flags for HasCursorKeys() */

  if (has_pointer) {
    if (pointer == nullptr) {
      pointer = ::wl_seat_get_pointer(seat);
      if (pointer != nullptr)
        ::wl_pointer_add_listener(pointer, &pointer_listener, this);
    }
  } else {
    if (pointer != nullptr) ::wl_pointer_destroy(pointer);
  }

  if (has_keyboard) {
    if (keyboard == nullptr) {
      keyboard = ::wl_seat_get_keyboard(seat);
      if (keyboard != nullptr)
        ::wl_keyboard_add_listener(keyboard, &keyboard_listener, this);
    }
  } else {
    if (keyboard != nullptr) ::wl_keyboard_destroy(keyboard);
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
  if (new_pointer_position == pointer_position) return;

  pointer_position = new_pointer_position;
  PixelPoint p(pointer_position.x, pointer_position.y);
#ifdef SOFTWARE_ROTATE_DISPLAY
  p = TransformCoordinates(p, physical_screen_size);
#endif
  Push(Event(Event::MOUSE_MOTION, p));
}

inline void
WaylandEventQueue::PointerButton(bool pressed) noexcept
{
  PixelPoint p(pointer_position.x, pointer_position.y);
#ifdef SOFTWARE_ROTATE_DISPLAY
  p = TransformCoordinates(p, physical_screen_size);
#endif
  Push(Event(pressed ? Event::MOUSE_DOWN : Event::MOUSE_UP, p));
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

void
WaylandEventQueue::SetCursor(struct wl_pointer *wl_pointer,
                             uint32_t serial) noexcept
{
  if (cursor_pointer == nullptr || cursor_surface == nullptr) return;

  if (cursor_pointer->image_count == 0) return;

  struct ::wl_cursor_image *cursor_image = cursor_pointer->images[0];
  if (cursor_image == nullptr) return;

  ::wl_pointer_set_cursor(wl_pointer, serial, cursor_surface,
                        cursor_image->hotspot_x, cursor_image->hotspot_y);

  struct ::wl_buffer *buffer = ::wl_cursor_image_get_buffer(cursor_image);
  if (buffer != nullptr) {
    ::wl_surface_attach(cursor_surface, buffer, 0, 0);
    ::wl_surface_damage(cursor_surface, 0, 0, cursor_image->width,
                      cursor_image->height);
    ::wl_surface_commit(cursor_surface);
  }
}

#ifdef SOFTWARE_ROTATE_DISPLAY

PixelPoint
WaylandEventQueue::GetTransformedPointerPosition() const noexcept
{
  PixelPoint p(pointer_position.x, pointer_position.y);
  return TransformCoordinates(p, physical_screen_size);
}

void
WaylandEventQueue::SetScreenSize(PixelSize screen_size) noexcept
{
  if (AreAxesSwapped(OpenGL::display_orientation)) {
    physical_screen_size = PixelSize(screen_size.height, screen_size.width);
  } else {
    physical_screen_size = screen_size;
  }
}

void
WaylandEventQueue::SetDisplayOrientation(
    [[maybe_unused]] DisplayOrientation orientation) noexcept
{
}

WaylandEventQueue::~WaylandEventQueue() noexcept
{
  if (cursor_surface != nullptr)
    ::wl_surface_destroy(cursor_surface);
  if (cursor_theme != nullptr)
    ::wl_cursor_theme_destroy(cursor_theme);
  if (xkb_state != nullptr)
    ::xkb_state_unref(xkb_state);
  if (xkb_keymap != nullptr)
    ::xkb_keymap_unref(xkb_keymap);
  if (xkb_context != nullptr)
    ::xkb_context_unref(xkb_context);
  if (keyboard != nullptr)
    ::wl_keyboard_destroy(keyboard);
  if (pointer != nullptr)
    ::wl_pointer_destroy(pointer);
  if (seat != nullptr)
    ::wl_seat_destroy(seat);
  if (decoration_manager != nullptr)
    ::zxdg_decoration_manager_v1_destroy(decoration_manager);
  if (wm_base != nullptr)
    ::xdg_wm_base_destroy(wm_base);
  if (shell != nullptr)
    ::wl_shell_destroy(shell);
  if (compositor != nullptr)
    ::wl_compositor_destroy(compositor);
  if (display != nullptr)
    ::wl_display_disconnect(display);
}

#endif

} // namespace UI
