// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef ENABLE_OPENGL
#include "ui/opengl/Features.hpp"
#endif
#include "WaylandQueue.hpp"
#include "Queue.hpp"
#include "../shared/Event.hpp"
#include "ui/display/Display.hpp"
#include "ui/display/wayland/Scale.hpp"
#include "ui/dim/Point.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "util/StringAPI.hxx"
#include "util/EnvParser.hpp"
#include "ui/event/poll/linux/Translate.hpp"
#include "ui/event/KeyCode.hpp"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "fractional-scale-v1-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "../shared/TransformCoordinates.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/dim/Size.hpp"
#endif

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio> // for fprintf()
#include <cstdlib> // for abort()
#include <cstring> // for strerror()
#include <stdexcept>

namespace UI {

[[gnu::const]]
static uint32_t
MinVersion(uint32_t advertised, uint32_t supported) noexcept
{
  return advertised < supported ? advertised : supported;
}

static void
WaylandRegistryGlobal(void *data, struct wl_registry *registry, uint32_t id,
                      const char *interface, [[maybe_unused]] uint32_t version)
{
  auto &q = *(WaylandEventQueue *)data;
  q.RegistryHandler(registry, id, interface, version);
}

static void
WaylandRegistryGlobalRemove(void *data,
                            [[maybe_unused]] struct wl_registry *registry,
                            uint32_t id)
{
  auto &queue = *(WaylandEventQueue *)data;
  /* Handle removal of globals. The seat may be removed if the input
   * device is disconnected. In that case, we should clean up pointer
   * and keyboard objects. However, we don't track which id corresponds
   * to which global, so we can't directly match. The compositor will
   * send a capabilities event with no capabilities when the seat is
   * removed, which will trigger cleanup via SeatHandleCapabilities. */
  (void)queue;
  (void)id;
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
                    uint32_t serial,
                    [[maybe_unused]] struct wl_surface *surface,
                    wl_fixed_t surface_x,
                    wl_fixed_t surface_y)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerEnter(serial,
                     IntPoint2D(wl_fixed_to_int(surface_x),
                                wl_fixed_to_int(surface_y)));
}

static void
WaylandPointerLeave(void *data,
                    [[maybe_unused]] struct wl_pointer *wl_pointer,
                    [[maybe_unused]] uint32_t serial,
                    [[maybe_unused]] struct wl_surface *surface)
{
  auto &queue = *(WaylandEventQueue *)data;
  queue.PointerLeave();
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
                     uint32_t serial,
                     [[maybe_unused]] uint32_t time,
                     uint32_t button,
                     uint32_t state)
{
  auto &queue = *(WaylandEventQueue *)data;

  queue.PointerButton(serial, button, state);
}

static void
WaylandPointerAxis(void *data,
                   [[maybe_unused]] struct wl_pointer *wl_pointer,
                   [[maybe_unused]] uint32_t time,
                   uint32_t axis, wl_fixed_t value)
{
  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL ||
      axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
    auto &q = *(WaylandEventQueue *)data;
    PixelPoint p = q.MaybeTransformPoint(q.GetPointerPosition());
    Event e(Event::MOUSE_WHEEL, p);
    /* Wayland's positive vertical axis is down. XCSoar matches X11:
       a positive wheel is up. */
    int delta = wl_fixed_to_int(value);
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
      delta = -delta;
    e.param = unsigned(delta);
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
WaylandTouchDown(void *data,
                  [[maybe_unused]] struct wl_touch *wl_touch,
                  [[maybe_unused]] uint32_t serial,
                  [[maybe_unused]] uint32_t time,
                  [[maybe_unused]] struct wl_surface *surface,
                  int32_t id,
                  wl_fixed_t x, wl_fixed_t y) noexcept
{
  auto &queue = *(WaylandEventQueue *)data;
  queue.TouchDown(id, PixelPoint(wl_fixed_to_int(x),
                                  wl_fixed_to_int(y)));
}

static void
WaylandTouchUp(void *data,
               [[maybe_unused]] struct wl_touch *wl_touch,
               [[maybe_unused]] uint32_t serial,
               [[maybe_unused]] uint32_t time,
               int32_t id) noexcept
{
  auto &queue = *(WaylandEventQueue *)data;
  queue.TouchUp(id);
}

static void
WaylandTouchMotion(void *data,
                    [[maybe_unused]] struct wl_touch *wl_touch,
                    [[maybe_unused]] uint32_t time,
                    int32_t id,
                    wl_fixed_t x, wl_fixed_t y) noexcept
{
  auto &queue = *(WaylandEventQueue *)data;
  queue.TouchMotion(id, PixelPoint(wl_fixed_to_int(x),
                                   wl_fixed_to_int(y)));
}

static void
WaylandTouchFrame(void *data,
                  [[maybe_unused]] struct wl_touch *wl_touch) noexcept
{
  auto &queue = *(WaylandEventQueue *)data;
  queue.TouchFrame();
}

static void
WaylandTouchCancel(void *data,
                   [[maybe_unused]] struct wl_touch *wl_touch) noexcept
{
  auto &queue = *(WaylandEventQueue *)data;
  queue.TouchCancel();
}

static constexpr struct wl_touch_listener touch_listener = {
  .down = WaylandTouchDown,
  .up = WaylandTouchUp,
  .motion = WaylandTouchMotion,
  .frame = WaylandTouchFrame,
  .cancel = WaylandTouchCancel,
};

static void
WaylandKeyboardKeymap(void *data,
                      [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                      uint32_t format,
                      int32_t fd,
                      uint32_t size) noexcept
{
  auto &queue = *(WaylandEventQueue *)data;
  queue.KeyboardKeymap(format, fd, size);
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
WaylandKeyboardModifiers(void *data,
                         [[maybe_unused]] struct wl_keyboard *wl_keyboard,
                         [[maybe_unused]] uint32_t serial,
                         uint32_t mods_depressed,
                         uint32_t mods_latched,
                         uint32_t mods_locked,
                         uint32_t group) noexcept
{
  auto &queue = *(WaylandEventQueue *)data;
  queue.KeyboardModifiers(mods_depressed, mods_latched, mods_locked, group);
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
   ui_display(_display),
   display(_display.GetWaylandDisplay()),
   scale_120(_display.GetScale120() > 0 ? _display.GetScale120() : 120u),
   socket_event(queue.GetEventLoop(), BIND_THIS_METHOD(OnSocketReady)),
   flush_event(queue.GetEventLoop(), BIND_THIS_METHOD(OnFlush))
{
  if (display == nullptr)
    throw std::runtime_error("wl_display_connect() failed");

  xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (xkb_context == nullptr)
    throw std::runtime_error("xkb_context_new() failed");

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

  if (shm == nullptr) throw std::runtime_error("No Wayland wl_shm found");

  /* Load cursor theme. Use nullptr for theme name to use the default system
     theme. Cursor size: read from XCURSOR_SIZE environment variable
     (standard), or calculate from display DPI if available. Defaults to 24
     pixels for 96 DPI displays, scaling proportionally for higher DPI. */
  /* Allow up to 2048 pixels to support very high-DPI displays (e.g., 3000 DPI
     -> ~750px). Cursor themes typically have sizes up to 256px, but we allow
     larger requests; the theme will use the closest available size. */
  int cursor_size = GetEnvInt("XCURSOR_SIZE", 0, /* min */ 0, /* max */ 2048);
  /* XCURSOR_SIZE is a logical size.  ApplyCursor() sets the buffer
     scale and divides the hotspot, so the image must be that many
     times larger.  The DPI fallback below is already in device pixels. */
  xcursor_size = cursor_size;
  if (cursor_size == 0) {
    /* If XCURSOR_SIZE not set, scale default cursor size based on DPI
       Default is 24px at 96 DPI (1/4 inch). Scale proportionally for
       higher DPI displays (e.g., 650 DPI -> ~162px, 3000 DPI -> ~750px). */
    const auto dpi = ::Display::GetDPI(_display, 0);
    if (dpi.x > 0) {
      /* Scale: cursor_size = 24 * (dpi / 96) */
      const int scaled_size = (24 * dpi.x + 48) / 96; /* +48 for rounding */
      if (scaled_size > 0 && scaled_size <= 2048) {
        cursor_size = scaled_size;
      } else {
        cursor_size = 24; /* Fallback to default if scaling fails */
      }
    } else {
      cursor_size = 24; /* Fallback to default if DPI unavailable */
    }
  } else {
    loaded_cursor_scale = Wayland::ToIntegerScale(scale_120);
    const int scaled = cursor_size * (int)loaded_cursor_scale;
    cursor_size = scaled > 2048 ? 2048 : scaled;
  }
  cursor_theme = wl_cursor_theme_load(nullptr, cursor_size, shm);
  if (cursor_theme != nullptr) {
    cursor_pointer = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
    if (cursor_pointer == nullptr) {
      /* Fallback to default cursor name */
      cursor_pointer = wl_cursor_theme_get_cursor(cursor_theme, "default");
    }
    if (cursor_pointer != nullptr && compositor != nullptr) {
      cursor_surface = wl_compositor_create_surface(compositor);
    }
  }

  socket_event.Open(SocketDescriptor(wl_display_get_fd(display)));
  socket_event.ScheduleRead();
  flush_event.Schedule();
}

WaylandEventQueue::~WaylandEventQueue() noexcept
{
  /* Clean up xkbcommon resources */
  if (xkb_state != nullptr) {
    xkb_state_unref(xkb_state);
    xkb_state = nullptr;
  }
  if (xkb_keymap != nullptr) {
    xkb_keymap_unref(xkb_keymap);
    xkb_keymap = nullptr;
  }
  if (xkb_context != nullptr) {
    xkb_context_unref(xkb_context);
    xkb_context = nullptr;
  }

  /* Clean up cursor resources */
  if (cursor_surface != nullptr) {
    wl_surface_destroy(cursor_surface);
    cursor_surface = nullptr;
  }
  if (cursor_theme != nullptr) {
    wl_cursor_theme_destroy(cursor_theme);
    cursor_theme = nullptr;
  }

  /* Clean up input devices */
  if (pointer != nullptr) {
    wl_pointer_destroy(pointer);
    pointer = nullptr;
  }
  if (keyboard != nullptr) {
    wl_keyboard_destroy(keyboard);
    keyboard = nullptr;
  }
  DestroyTouch();

  /* Clean up Wayland protocol objects obtained via wl_registry_bind */
  /* Use wl_proxy_destroy for global objects (compositor, seat, shell, shm) */
  /* Use protocol-specific destroy functions for xdg and zxdg objects */
  if (pointer_constraints != nullptr) {
    zwp_pointer_constraints_v1_destroy(pointer_constraints);
    pointer_constraints = nullptr;
  }
  if (fractional_scale_manager != nullptr) {
    wp_fractional_scale_manager_v1_destroy(fractional_scale_manager);
    fractional_scale_manager = nullptr;
  }
  if (viewporter != nullptr) {
    wp_viewporter_destroy(viewporter);
    viewporter = nullptr;
  }
  if (decoration_manager != nullptr) {
    zxdg_decoration_manager_v1_destroy(decoration_manager);
    decoration_manager = nullptr;
  }
  if (wm_base != nullptr) {
    xdg_wm_base_destroy(wm_base);
    wm_base = nullptr;
  }
  if (shell != nullptr) {
    wl_proxy_destroy((struct wl_proxy *)shell);
    shell = nullptr;
  }
  if (shm != nullptr) {
    wl_proxy_destroy((struct wl_proxy *)shm);
    shm = nullptr;
  }
  if (seat != nullptr) {
    wl_proxy_destroy((struct wl_proxy *)seat);
    seat = nullptr;
  }
  if (compositor != nullptr) {
    wl_proxy_destroy((struct wl_proxy *)compositor);
    compositor = nullptr;
  }
  /* Note: display is owned by UI::Display, not by WaylandEventQueue.
   * The Display destructor will call wl_display_disconnect(), so we
   * must not disconnect it here to avoid double-disconnect segfault. */
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

bool
WaylandEventQueue::IsVisible() const noexcept
{
  /* Unfocused is not pause.  Skip Flip() only while SUSPENDED. */
  return !suspended;
}

void
WaylandEventQueue::SetToplevelState(bool new_suspended) noexcept
{
  const bool was_visible = IsVisible();
  suspended = new_suspended;

  if (IsVisible() && !was_visible)
    queue.Push(Event::EXPOSE);
}

inline void
WaylandEventQueue::RegistryHandler(struct wl_registry *registry, uint32_t id,
                                   const char *interface,
                                   uint32_t version) noexcept
{
  if (interface == nullptr || *interface == '\0')
    return;

  if (StringIsEqual(interface, "wl_compositor")) {
    compositor = (wl_compositor *)
      wl_registry_bind(registry, id, &wl_compositor_interface,
                        MinVersion(version,
                                   (uint32_t)wl_compositor_interface.version));
  } else if (StringIsEqual(interface, "wl_seat")) {
    seat = (wl_seat *)wl_registry_bind(registry, id,
                                         &wl_seat_interface, 1);
    wl_seat_add_listener(seat, &seat_listener, this);
  } else if (StringIsEqual(interface, "wl_shm"))
    shm = (wl_shm *)wl_registry_bind(registry, id,
                                      &wl_shm_interface, 1);
  else if (StringIsEqual(interface, "wl_shell"))
    shell = (wl_shell *)wl_registry_bind(registry, id,
                                         &wl_shell_interface, 1);
  else if (StringIsEqual(interface, "xdg_wm_base")) {
    wm_base = (xdg_wm_base *)
      wl_registry_bind(registry, id, &xdg_wm_base_interface,
                        MinVersion(version,
                                   (uint32_t)xdg_wm_base_interface.version));
  } else if (StringIsEqual(interface, "zxdg_decoration_manager_v1"))
    decoration_manager = (zxdg_decoration_manager_v1 *)
      wl_registry_bind(registry, id,
                       &zxdg_decoration_manager_v1_interface, 1);
  else if (StringIsEqual(interface, wp_viewporter_interface.name) &&
           viewporter == nullptr && version >= 1)
    viewporter = (struct wp_viewporter *)
      wl_registry_bind(registry, id, &wp_viewporter_interface, 1);
  else if (StringIsEqual(interface,
                          wp_fractional_scale_manager_v1_interface.name) &&
           fractional_scale_manager == nullptr && version >= 1)
    fractional_scale_manager = (struct wp_fractional_scale_manager_v1 *)
      wl_registry_bind(registry, id,
                       &wp_fractional_scale_manager_v1_interface, 1);
  else if (StringIsEqual(interface,
                          zwp_pointer_constraints_v1_interface.name) &&
           pointer_constraints == nullptr && version >= 1)
    pointer_constraints = (struct zwp_pointer_constraints_v1 *)
      wl_registry_bind(registry, id,
                       &zwp_pointer_constraints_v1_interface, 1);
}

inline void
WaylandEventQueue::SeatHandleCapabilities(bool has_pointer, bool has_keyboard,
                                          bool has_touch) noexcept
{
  if (has_pointer) {
    if (pointer == nullptr) {
      pointer = wl_seat_get_pointer(seat);
      if (pointer != nullptr)
        wl_pointer_add_listener(pointer, &pointer_listener, this);
    }
  } else {
    if (pointer != nullptr) {
      wl_pointer_destroy(pointer);
      pointer = nullptr;
    }
  }

  if (has_keyboard) {
    if (keyboard == nullptr) {
      keyboard = wl_seat_get_keyboard(seat);
      if (keyboard != nullptr)
        wl_keyboard_add_listener(keyboard, &keyboard_listener, this);
    }
  } else {
    if (keyboard != nullptr) {
      wl_keyboard_destroy(keyboard);
      keyboard = nullptr;
    }
  }

  if (has_touch) {
    if (touch == nullptr) {
      touch = wl_seat_get_touch(seat);
      if (touch != nullptr)
        wl_touch_add_listener(touch, &touch_listener, this);
    }
  } else if (touch != nullptr) {
    TouchCancel();
    DestroyTouch();
  }

  has_touchscreen = touch != nullptr;
}

inline void
WaylandEventQueue::Push(const Event &event) noexcept
{
  queue.Push(event);
}

PixelPoint
WaylandEventQueue::MaybeTransformPoint(PixelPoint p) const noexcept
{
#if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
  return TransformCoordinates(p, physical_screen_size);
#else
  return p;
#endif
}

PixelPoint
WaylandEventQueue::ScaleCompositorPoint(PixelPoint compositor) const noexcept
{
  return Wayland::ToPhysicalPoint(compositor, scale_120);
}

void
WaylandEventQueue::DestroyTouch() noexcept
{
  if (touch == nullptr)
    return;

  wl_touch_destroy(touch);
  touch = nullptr;
}

void
WaylandEventQueue::TouchDown(int32_t id, PixelPoint compositor) noexcept
{
  const PixelPoint p = ScaleCompositorPoint(compositor);
  for (unsigned i = 0; i < touches.size(); ++i) {
    if (touches[i].id == id) {
      touches[i].p = p;
      touches[i].up = false;
      return;
    }
  }

  touches.checked_append({id, p, false});
}

void
WaylandEventQueue::TouchUp(int32_t id) noexcept
{
  for (unsigned i = 0; i < touches.size(); ++i) {
    if (touches[i].id == id) {
      touches[i].up = true;
      return;
    }
  }
}

void
WaylandEventQueue::TouchMotion(int32_t id, PixelPoint compositor) noexcept
{
  const PixelPoint p = ScaleCompositorPoint(compositor);
  for (unsigned i = 0; i < touches.size(); ++i) {
    if (touches[i].id == id) {
      touches[i].p = p;
      return;
    }
  }
}

void
WaylandEventQueue::FlushTouchFrame() noexcept
{
  unsigned live = 0;
  PixelPoint live_pts[2]{};
  PixelPoint lift_p = PixelPoint(pointer_position.x, pointer_position.y);
  bool have_lift = false;
  for (unsigned i = 0; i < touches.size(); ++i) {
    if (!touches[i].up) {
      if (live < 2)
        live_pts[live] = touches[i].p;
      ++live;
    } else if (!have_lift) {
      lift_p = touches[i].p;
      have_lift = true;
    }
  }

  const unsigned prev = last_touch_count;
  if (live == 0 && prev == 0 && touches.empty())
    return;

  if (live >= 2)
    touch_multi = true;

  if (live >= 1)
    pointer_position = IntPoint2D(live_pts[0].x, live_pts[0].y);
  else if (have_lift)
    pointer_position = IntPoint2D(lift_p.x, lift_p.y);

  const PixelPoint p0 = MaybeTransformPoint(PixelPoint(pointer_position.x,
                                                         pointer_position.y));

  if (prev == 0 && (live >= 1 || !touches.empty()))
    Push(Event(Event::MOUSE_DOWN, p0));

  if (live >= 2) {
    const PixelPoint a = MaybeTransformPoint(live_pts[0]);
    const PixelPoint b = MaybeTransformPoint(live_pts[1]);
    if (prev < 2)
      Push(Event(Event::POINTER_DOWN, a, b));
    else {
      queue.Purge(Event::POINTER_MOVE);
      Push(Event(Event::POINTER_MOVE, a, b));
    }
  } else {
    if (prev >= 2)
      Push(Event(Event::POINTER_UP));
    if (live == 1 && prev == 1) {
      queue.Purge(Event::MOUSE_MOTION);
      Push(Event(Event::MOUSE_MOTION, p0));
    }
    if (live == 0 && (prev >= 1 || !touches.empty())) {
      Push(Event(touch_multi ? Event::MOUSE_CANCEL : Event::MOUSE_UP, p0));
      touch_multi = false;
    }
  }

  for (unsigned i = touches.size(); i > 0; --i) {
    if (touches[i - 1].up)
      touches.remove(i - 1);
  }

  last_touch_count = live;
}

void
WaylandEventQueue::TouchFrame() noexcept
{
  FlushTouchFrame();
}

void
WaylandEventQueue::TouchCancel() noexcept
{
  touches.clear();
  if (last_touch_count >= 2)
    Push(Event(Event::POINTER_UP));
  if (last_touch_count >= 1)
    Push(Event(Event::MOUSE_CANCEL));
  last_touch_count = 0;
  touch_multi = false;
}

namespace {

/* Logical pixels.  The compositor corner is 1px; this square is the
   target that starts a diagonal resize. */
constexpr unsigned CORNER_RESIZE_LOGICAL_PX = 16;

[[nodiscard]] [[gnu::const]]
constexpr uint32_t
CornerResizeEdge(int x, int y, int width, int height,
                 int margin) noexcept
{
  if (margin < 1 || width <= margin * 2 || height <= margin * 2)
    return XDG_TOPLEVEL_RESIZE_EDGE_NONE;

  uint32_t edge;
  if (x < margin)
    edge = XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
  else if (x >= width - margin)
    edge = XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
  else
    return XDG_TOPLEVEL_RESIZE_EDGE_NONE;

  if (y < margin)
    return edge | XDG_TOPLEVEL_RESIZE_EDGE_TOP;
  if (y >= height - margin)
    return edge | XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
  return XDG_TOPLEVEL_RESIZE_EDGE_NONE;
}

static_assert(CornerResizeEdge(0, 0, 100, 100, 16) ==
              XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT);
static_assert(CornerResizeEdge(90, 0, 100, 100, 16) ==
              XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT);
static_assert(CornerResizeEdge(0, 90, 100, 100, 16) ==
              XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT);
static_assert(CornerResizeEdge(90, 90, 100, 100, 16) ==
              XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT);
static_assert(CornerResizeEdge(50, 0, 100, 100, 16) ==
              XDG_TOPLEVEL_RESIZE_EDGE_NONE);
static_assert(CornerResizeEdge(0, 50, 100, 100, 16) ==
              XDG_TOPLEVEL_RESIZE_EDGE_NONE);
static_assert(CornerResizeEdge(0, 0, 20, 100, 16) ==
              XDG_TOPLEVEL_RESIZE_EDGE_NONE);

} // namespace

inline void
WaylandEventQueue::PointerEnter(uint32_t serial, IntPoint2D p) noexcept
{
  pointer_enter_serial = serial;
  shown_resize_edge = ~uint32_t{0};
  PointerMotion(p);
}

inline void
WaylandEventQueue::PointerLeave() noexcept
{
  shown_resize_edge = ~uint32_t{0};
}

inline void
WaylandEventQueue::PointerMotion(IntPoint2D new_pointer_position) noexcept
{
  if (last_touch_count > 0)
    return;

  new_pointer_position =
    Wayland::ToPhysicalPoint(PixelPoint(new_pointer_position), scale_120);

  const bool moved = new_pointer_position != pointer_position;
  pointer_position = new_pointer_position;
  UpdateCornerCursor();
  if (!moved)
    return;

  const PixelPoint transformed =
    MaybeTransformPoint(PixelPoint(pointer_position.x,
                                   pointer_position.y));
  Push(Event(Event::MOUSE_MOTION,
             transformed));
}

inline void
WaylandEventQueue::PointerButton(uint32_t serial, uint32_t button,
                                 uint32_t state) noexcept
{
  if (last_touch_count > 0)
    return;

  if (button == BTN_LEFT &&
      state == WL_POINTER_BUTTON_STATE_RELEASED &&
      swallow_corner_release) {
    swallow_corner_release = false;
    return;
  }

  /* A new left press retires a release the compositor never delivered. */
  if (state == WL_POINTER_BUTTON_STATE_PRESSED && button == BTN_LEFT) {
    swallow_corner_release = false;
    if (BeginCornerResize(serial)) {
      swallow_corner_release = true;
      return;
    }
  }

  const PixelPoint transformed =
    MaybeTransformPoint(PixelPoint(pointer_position.x,
                                   pointer_position.y));
  Push(Event(state == WL_POINTER_BUTTON_STATE_PRESSED
               ? Event::MOUSE_DOWN : Event::MOUSE_UP,
             transformed));
}

uint32_t
WaylandEventQueue::CurrentCornerEdge() const noexcept
{
  if (!resize_enabled || resize_size.width == 0 || resize_size.height == 0)
    return XDG_TOPLEVEL_RESIZE_EDGE_NONE;

  const int margin =
    (int)Wayland::ToPhysicalPixels(CORNER_RESIZE_LOGICAL_PX, scale_120);
  return CornerResizeEdge(pointer_position.x, pointer_position.y,
                          (int)resize_size.width, (int)resize_size.height,
                          margin);
}

struct wl_cursor *
WaylandEventQueue::CornerCursor(uint32_t edge) const noexcept
{
  if (cursor_theme == nullptr)
    return nullptr;

  const char *name;
  const char *fallback;
  switch (edge) {
  case XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT:
    name = "top_left_corner";
    fallback = "nw-resize";
    break;
  case XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT:
    name = "top_right_corner";
    fallback = "ne-resize";
    break;
  case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT:
    name = "bottom_left_corner";
    fallback = "sw-resize";
    break;
  case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT:
    name = "bottom_right_corner";
    fallback = "se-resize";
    break;
  default:
    return nullptr;
  }

  struct wl_cursor *cursor = wl_cursor_theme_get_cursor(cursor_theme, name);
  if (cursor == nullptr)
    cursor = wl_cursor_theme_get_cursor(cursor_theme, fallback);
  return cursor;
}

void
WaylandEventQueue::UpdateCornerCursor() noexcept
{
  EnsureCursorThemeScale();

  const uint32_t edge = CurrentCornerEdge();
  if (edge == shown_resize_edge)
    return;

  /* The frame keeps its last resize glyph until the client sets one.
     Inside the window, that glyph is the arrow, except on a corner. */
  if (edge != XDG_TOPLEVEL_RESIZE_EDGE_NONE &&
      ApplyCursor(CornerCursor(edge))) {
    shown_resize_edge = edge;
    return;
  }

  if (ApplyCursor(cursor_pointer))
    shown_resize_edge = XDG_TOPLEVEL_RESIZE_EDGE_NONE;
}

bool
WaylandEventQueue::BeginCornerResize(uint32_t serial) noexcept
{
  if (resize_toplevel == nullptr || seat == nullptr)
    return false;

  const uint32_t edge = CurrentCornerEdge();
  if (edge == XDG_TOPLEVEL_RESIZE_EDGE_NONE)
    return false;

  xdg_toplevel_resize(resize_toplevel, seat, serial, edge);
  return true;
}

void
WaylandEventQueue::KeyboardKeymap(uint32_t format, int32_t fd, uint32_t size) noexcept
{
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    close(fd);
    return;
  }

  char *map_shm = static_cast<char *>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
  if (map_shm == MAP_FAILED) {
    close(fd);
    return;
  }

  if (xkb_keymap != nullptr)
    xkb_keymap_unref(xkb_keymap);
  if (xkb_state != nullptr)
    xkb_state_unref(xkb_state);

  xkb_keymap = xkb_keymap_new_from_string(xkb_context, map_shm,
                                           XKB_KEYMAP_FORMAT_TEXT_V1,
                                           XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(map_shm, size);
  close(fd);

  if (xkb_keymap != nullptr)
    xkb_state = xkb_state_new(xkb_keymap);
}

void
WaylandEventQueue::KeyboardModifiers(uint32_t mods_depressed, uint32_t mods_latched,
                                     uint32_t mods_locked, uint32_t group) noexcept
{
  if (xkb_state != nullptr)
    xkb_state_update_mask(xkb_state, mods_depressed, mods_latched,
                          mods_locked, 0, 0, group);
}

void
WaylandEventQueue::KeyboardKey(uint32_t key, uint32_t state) noexcept
{
  unsigned key_code = 0;
  bool is_char = false;

  /* Linux evdev key codes for arrow keys (from linux/input-event-codes.h) */
  constexpr uint32_t EVDEV_KEY_UP = 103;
  constexpr uint32_t EVDEV_KEY_DOWN = 108;
  constexpr uint32_t EVDEV_KEY_LEFT = 105;
  constexpr uint32_t EVDEV_KEY_RIGHT = 106;

  /* Check if this is an arrow key BEFORE translation. Linux input event
     codes KEY_UP=103, KEY_DOWN=108, KEY_LEFT=105, KEY_RIGHT=106 conflict
     with ASCII 'g', 'l', 'i', 'j', so we must check the raw key code
     first to distinguish arrow keys from letters */
  if (key == EVDEV_KEY_UP) {
    key_code = KEY_UP;
    is_char = false;
  } else if (key == EVDEV_KEY_DOWN) {
    key_code = KEY_DOWN;
    is_char = false;
  } else if (key == EVDEV_KEY_LEFT) {
    key_code = KEY_LEFT;
    is_char = false;
  } else if (key == EVDEV_KEY_RIGHT) {
    key_code = KEY_RIGHT;
    is_char = false;
  } else {
    /* Normal key - use XKB to get the actual character for text input.
       XKB keycode = Linux evdev keycode + 8 (historical X11 offset) */
    uint32_t xkb_keycode = key + 8;

    if (xkb_state != nullptr && xkb_keymap != nullptr) {
      uint32_t utf32 = xkb_state_key_get_utf32(xkb_state, xkb_keycode);

      /* Check if it's a printable character: not control characters
         (>= 0x20, except DEL 0x7F), valid Unicode scalar value
         (<= 0x10FFFF, excluding surrogates 0xD800-0xDFFF) */
      bool is_printable = (utf32 >= 0x20) &&
                          (utf32 != 0x7F) &&
                          (utf32 < 0xD800 || utf32 > 0xDFFF) &&
                          (utf32 <= 0x10FFFF);

      if (is_printable) {
        key_code = utf32;
        is_char = true;
      } else {
        const auto [translated_key_code, is_char_result] = TranslateKeyCode(key);
        key_code = translated_key_code;
        is_char = is_char_result;
      }
    } else {
      const auto [translated_key_code, is_char_result] = TranslateKeyCode(key);
      key_code = translated_key_code;
      is_char = is_char_result;
    }
  }

  switch (state) {
  case WL_KEYBOARD_KEY_STATE_RELEASED:
    {
      Event e(Event::KEY_UP, key_code);
      e.is_char = is_char;
      queue.Push(e);
    }
    break;

  case WL_KEYBOARD_KEY_STATE_PRESSED:
    {
      Event e(Event::KEY_DOWN, key_code);
      e.is_char = is_char;
      queue.Push(e);
    }
    break;
  }
}

#ifdef SOFTWARE_ROTATE_DISPLAY

void
WaylandEventQueue::SetScreenSize(PixelSize screen_size) noexcept
{
  physical_screen_size = screen_size;
}

#endif

void
WaylandEventQueue::EnsureCursorThemeScale() noexcept
{
  if (xcursor_size <= 0 || shm == nullptr)
    return;

  const unsigned scale = Wayland::ToIntegerScale(scale_120);
  if (scale == loaded_cursor_scale)
    return;

  int size = xcursor_size * (int)scale;
  if (size > 2048)
    size = 2048;
  if (size < 1)
    size = 1;

  if (cursor_theme != nullptr)
    wl_cursor_theme_destroy(cursor_theme);
  cursor_theme = wl_cursor_theme_load(nullptr, size, shm);
  cursor_pointer = nullptr;
  if (cursor_theme != nullptr) {
    cursor_pointer = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
    if (cursor_pointer == nullptr)
      cursor_pointer = wl_cursor_theme_get_cursor(cursor_theme, "default");
  }
  loaded_cursor_scale = scale;
  shown_resize_edge = ~uint32_t{0};
}

bool
WaylandEventQueue::ApplyCursor(struct wl_cursor *cursor) noexcept
{
  if (cursor == nullptr || cursor_surface == nullptr || pointer == nullptr)
    return false;

  if (cursor->image_count == 0)
    return false;

  struct wl_cursor_image *image = cursor->images[0];
  if (image == nullptr)
    return false;

  /* Theme pixels are already device pixels.  Without buffer scale the
     compositor enlarges them again on a scaled output. */
  const unsigned scale = Wayland::ToIntegerScale(scale_120);
  if (wl_proxy_get_version((struct wl_proxy *)cursor_surface) >= 3)
    wl_surface_set_buffer_scale(cursor_surface, (int32_t)scale);

  wl_pointer_set_cursor(pointer, pointer_enter_serial, cursor_surface,
                        (int32_t)(image->hotspot_x / scale),
                        (int32_t)(image->hotspot_y / scale));

  struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);
  if (buffer == nullptr)
    return false;

  wl_surface_attach(cursor_surface, buffer, 0, 0);
  wl_surface_damage(cursor_surface, 0, 0, image->width, image->height);
  wl_surface_commit(cursor_surface);
  return true;
}

} // namespace UI
