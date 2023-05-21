// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  struct wl_shell *shell = nullptr;

  IntPoint2D pointer_position = {0, 0};

  SocketEvent socket_event;

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

  bool IsVisible() const noexcept {
    // TODO: implement
    return true;
  }

  bool Generate(Event &event) noexcept;

  void RegistryHandler(struct wl_registry *registry, uint32_t id,
                       const char *interface) noexcept;

  void SeatHandleCapabilities(bool pointer, bool keyboard, bool touch) noexcept;

  void Push(const Event &event) noexcept;
  void PointerMotion(IntPoint2D new_pointer_position) noexcept;
  void PointerButton(bool pressed) noexcept;

private:
  void OnSocketReady(unsigned events) noexcept;
};

} // namespace UI
