// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "event/SocketEvent.hxx"
#include "event/IdleEvent.hxx"
#include "Math/Point2D.hpp"

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "ui/dim/Size.hpp"
#endif

#include <cstdint>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;

struct PixelSize;

struct wl_display;
struct wl_compositor;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_shell;
struct wl_registry;
struct wl_shm;
struct wl_surface;
struct wl_cursor_theme;
struct wl_cursor;
struct xdg_wm_base;
struct zxdg_decoration_manager_v1;

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

  struct wl_display *const display;
  struct wl_compositor *compositor = nullptr;
  struct wl_seat *seat = nullptr;
  struct wl_pointer *pointer = nullptr;
  struct wl_keyboard *keyboard = nullptr;
  struct wl_shell *shell = nullptr;
  struct xdg_wm_base *wm_base = nullptr;
  struct zxdg_decoration_manager_v1 *decoration_manager = nullptr;
  struct wl_shm *shm = nullptr;

  bool has_touchscreen = false;

  /* Cursor support */
  struct wl_cursor_theme *cursor_theme = nullptr;
  struct wl_cursor *cursor_pointer = nullptr;
  struct wl_surface *cursor_surface = nullptr;

  IntPoint2D pointer_position = {0, 0};

  struct xkb_context *xkb_context = nullptr;
  struct xkb_keymap *xkb_keymap = nullptr;
  struct xkb_state *xkb_state = nullptr;

  SocketEvent socket_event;
  IdleEvent flush_event;

#ifdef SOFTWARE_ROTATE_DISPLAY
  PixelSize physical_screen_size{0, 0};
#endif

  /**
   * Bound xdg_wm_base version.  State `suspended` exists since
   * version 6.
   */
  uint32_t xdg_wm_base_version = 0;

  bool activated = true;
  bool suspended = false;

  /**
   * True after the first buffer was presented.  Until then the
   * surface is not mapped, so missing ACTIVATED must not skip
   * eglSwapBuffers or the window never appears.
   */
  bool presented = false;

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

  struct wl_pointer *GetPointer() const noexcept {
    return pointer;
  }

  [[gnu::pure]]
  bool IsVisible() const noexcept;

  /**
   * Update xdg_toplevel configure states.  Hidden surfaces must not
   * call eglSwapBuffers: Mesa waits for a frame callback the
   * compositor will not send.
   */
  void SetToplevelState(bool activated, bool suspended) noexcept;

  void MarkPresented() noexcept {
    presented = true;
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
  void PointerMotion(IntPoint2D new_pointer_position) noexcept;
  void PointerButton(bool pressed) noexcept;

  void KeyboardKey(uint32_t key, uint32_t state) noexcept;
  void KeyboardKeymap(uint32_t format, int32_t fd, uint32_t size) noexcept;
  void KeyboardModifiers(uint32_t mods_depressed, uint32_t mods_latched,
                         uint32_t mods_locked, uint32_t group) noexcept;

  void SetCursor(struct wl_pointer *wl_pointer, uint32_t serial) noexcept;

private:
  void OnSocketReady(unsigned events) noexcept;
  void OnFlush() noexcept;
};

} // namespace UI
