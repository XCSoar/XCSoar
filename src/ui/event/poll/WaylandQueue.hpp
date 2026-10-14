// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "event/SocketEvent.hxx"
#include "event/IdleEvent.hxx"
#include "Math/Point2D.hpp"
#include "ui/dim/Point.hpp"
#include "util/StaticArray.hxx"

#include "ui/dim/Size.hpp"

#include <cstdint>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;

struct wl_display;
struct wl_compositor;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_touch;
struct wl_shell;
struct wl_registry;
struct wl_shm;
struct wl_surface;
struct wl_cursor_theme;
struct wl_cursor;
struct xdg_wm_base;
struct xdg_toplevel;
struct zxdg_decoration_manager_v1;
struct wp_viewporter;
struct wp_fractional_scale_manager_v1;
struct zwp_pointer_constraints_v1;

namespace UI {

class Display;
class EventQueue;
struct Event;

/**
 * This class opens a connection to a Wayland compositor and
 * listens for input events.
 */
class WaylandEventQueue final {
  EventQueue &queue;

  UI::Display &ui_display;
  struct wl_display *const display;
  struct wl_compositor *compositor = nullptr;
  struct wl_seat *seat = nullptr;
  struct wl_pointer *pointer = nullptr;
  struct wl_keyboard *keyboard = nullptr;
  struct wl_touch *touch = nullptr;
  struct wl_shell *shell = nullptr;
  struct xdg_wm_base *wm_base = nullptr;
  struct zxdg_decoration_manager_v1 *decoration_manager = nullptr;
  struct wp_viewporter *viewporter = nullptr;
  struct wp_fractional_scale_manager_v1 *fractional_scale_manager = nullptr;
  struct zwp_pointer_constraints_v1 *pointer_constraints = nullptr;
  struct wl_shm *shm = nullptr;

  bool has_touchscreen = false;

  /* Cursor support */
  struct wl_cursor_theme *cursor_theme = nullptr;
  struct wl_cursor *cursor_pointer = nullptr;
  struct wl_surface *cursor_surface = nullptr;

  IntPoint2D pointer_position = {0, 0};

  /* Server-side decoration corners are the 1px crossing of the two
     edges.  A square inside the surface is the resize target. */
  struct xdg_toplevel *resize_toplevel = nullptr;
  PixelSize resize_size{0, 0};
  bool resize_enabled = false;
  uint32_t pointer_enter_serial = 0;
  uint32_t shown_resize_edge = ~uint32_t{0};
  bool swallow_corner_release = false;

  struct TouchContact {
    int32_t id;
    PixelPoint p;
    bool up = false;
  };

  StaticArray<TouchContact, 10> touches;
  unsigned last_touch_count = 0;
  bool touch_multi = false;
  unsigned scale_120 = 120;

  struct xkb_context *xkb_context = nullptr;
  struct xkb_keymap *xkb_keymap = nullptr;
  struct xkb_state *xkb_state = nullptr;

  SocketEvent socket_event;
  IdleEvent flush_event;

#ifdef SOFTWARE_ROTATE_DISPLAY
  PixelSize physical_screen_size{0, 0};
#endif

  bool suspended = false;

public:
  /**
   * @param queue the #EventQueue that shall receive Wayland input
   * events
   */
  WaylandEventQueue(UI::Display &display, EventQueue &queue);
  ~WaylandEventQueue() noexcept;

  struct wl_compositor *GetCompositor() const noexcept {
    return compositor;
  }

  struct wl_shell *GetShell() const noexcept {
    return shell;
  }

  struct xdg_wm_base *GetWmBase() const noexcept {
    return wm_base;
  }

  struct zxdg_decoration_manager_v1 *GetDecorationManager() const noexcept {
    return decoration_manager;
  }

  struct wp_viewporter *GetViewporter() const noexcept {
    return viewporter;
  }

  struct wp_fractional_scale_manager_v1 *
  GetFractionalScaleManager() const noexcept {
    return fractional_scale_manager;
  }

  struct zwp_pointer_constraints_v1 *
  GetPointerConstraints() const noexcept {
    return pointer_constraints;
  }

  struct wl_pointer *GetPointer() const noexcept {
    return pointer;
  }

  [[gnu::pure]]
  bool IsVisible() const noexcept;

  /**
   * Skip eglSwapBuffers only while SUSPENDED.  Unfocused is visible.
   */
  void SetToplevelState(bool suspended) noexcept;

  void SetSurfaceScale120(unsigned new_scale_120) noexcept {
    if (new_scale_120 > 0)
      scale_120 = new_scale_120;
  }

  void SetResizeToplevel(struct xdg_toplevel *toplevel) noexcept {
    resize_toplevel = toplevel;
    if (toplevel == nullptr)
      resize_enabled = false;
  }

  void SetResizeEnabled(bool enabled) noexcept {
    resize_enabled = resize_toplevel != nullptr && enabled;
  }

  void SetResizeSize(const PixelSize &size) noexcept {
    resize_size = size;
  }

  bool HasPointer() const noexcept {
    return pointer != nullptr;
  }

  bool HasTouchScreen() const noexcept {
    return has_touchscreen;
  }

  bool HasKeyboard() const noexcept {
    return keyboard != nullptr;
  }

  bool Generate(Event &event) noexcept;

#ifdef SOFTWARE_ROTATE_DISPLAY
  void SetScreenSize(PixelSize new_size) noexcept;
#endif

  void RegistryHandler(struct wl_registry *registry, uint32_t id,
                       const char *interface, uint32_t version) noexcept;

  void SeatHandleCapabilities(bool pointer, bool keyboard, bool touch) noexcept;

  void Push(const Event &event) noexcept;
  [[gnu::pure]]
  PixelPoint MaybeTransformPoint(PixelPoint p) const noexcept;
  [[gnu::pure]]
  PixelPoint GetPointerPosition() const noexcept {
    return PixelPoint(pointer_position.x, pointer_position.y);
  }
  void PointerEnter(uint32_t serial, IntPoint2D p) noexcept;
  void PointerLeave() noexcept;
  void PointerMotion(IntPoint2D new_pointer_position) noexcept;
  void PointerButton(uint32_t serial, uint32_t button,
                     uint32_t state) noexcept;

  void TouchDown(int32_t id, PixelPoint compositor) noexcept;
  void TouchUp(int32_t id) noexcept;
  void TouchMotion(int32_t id, PixelPoint compositor) noexcept;
  void TouchFrame() noexcept;
  void TouchCancel() noexcept;

  void KeyboardKey(uint32_t key, uint32_t state) noexcept;
  void KeyboardKeymap(uint32_t format, int32_t fd, uint32_t size) noexcept;
  void KeyboardModifiers(uint32_t mods_depressed, uint32_t mods_latched,
                         uint32_t mods_locked, uint32_t group) noexcept;

private:
  [[nodiscard]] uint32_t CurrentCornerEdge() const noexcept;
  void UpdateCornerCursor() noexcept;
  [[nodiscard]] bool BeginCornerResize(uint32_t serial) noexcept;
  [[nodiscard]] bool ApplyCursor(struct wl_cursor *cursor) noexcept;
  [[nodiscard]] struct wl_cursor *
  CornerCursor(uint32_t edge) const noexcept;
  [[gnu::pure]]
  PixelPoint ScaleCompositorPoint(PixelPoint compositor) const noexcept;
  void FlushTouchFrame() noexcept;
  void DestroyTouch() noexcept;

  void OnSocketReady(unsigned events) noexcept;
  void OnFlush() noexcept;
};

} // namespace UI
