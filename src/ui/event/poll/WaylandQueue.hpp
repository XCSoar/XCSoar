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

struct wl_display;
struct wl_compositor;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_shell;
struct wl_registry;
struct xdg_wm_base;

enum class DisplayOrientation : uint8_t;
struct PixelSize;

namespace UI {

class Display;
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
  struct wl_keyboard *keyboard = nullptr;
  struct wl_shell *shell = nullptr;
  struct xdg_wm_base *wm_base = nullptr;

  bool has_touchscreen = false;

  IntPoint2D pointer_position = {0, 0};

  SocketEvent socket_event;
  IdleEvent flush_event;

#ifdef SOFTWARE_ROTATE_DISPLAY
  PixelSize physical_screen_size{0, 0};
#endif

public:
  /**
   * @param queue the #EventQueue that shall receive Wayland input
   * events
   */
  WaylandEventQueue(UI::Display &display, EventQueue &queue);

  struct wl_compositor *GetCompositor() const noexcept {
    return compositor;
  }

  struct wl_shell *GetShell() const noexcept {
    return shell;
  }

  struct xdg_wm_base *GetWmBase() const noexcept {
    return wm_base;
  }

  bool IsVisible() const noexcept {
    // TODO: implement
    return true;
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

#ifdef SOFTWARE_ROTATE_DISPLAY
  void SetScreenSize(PixelSize screen_size) noexcept;
  void SetDisplayOrientation(DisplayOrientation orientation) noexcept;
#endif

  bool Generate(Event &event) noexcept;

  void RegistryHandler(struct wl_registry *registry, uint32_t id,
                       const char *interface) noexcept;

  void SeatHandleCapabilities(bool pointer, bool keyboard, bool touch) noexcept;

  void Push(const Event &event) noexcept;
  void PointerMotion(IntPoint2D new_pointer_position) noexcept;
  void PointerButton(bool pressed) noexcept;

#ifdef SOFTWARE_ROTATE_DISPLAY
  PixelPoint GetTransformedPointerPosition() const noexcept;
#endif

  void KeyboardKey(uint32_t key, uint32_t state) noexcept;

private:
  void OnSocketReady(unsigned events) noexcept;
  void OnFlush() noexcept;
};

} // namespace UI
