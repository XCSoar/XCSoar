// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "event/SocketEvent.hxx"

#ifdef USE_X11
#include "ui/opengl/Features.hpp"
#endif

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "../shared/RotatePointer.hpp"
#endif

/* kludges to work around namespace collisions with X11 headers */

#define Font X11Font
#define Window X11Window
#define Display X11Display

#include "X11/Xdefs.h"

#undef Font
#undef Window
#undef Display

struct _XDisplay;
union _XEvent;

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
class X11EventQueue {
  EventQueue &queue;

  _XDisplay *const display;

  SocketEvent socket_event;

  Atom wm_delete_window;

  bool mapped = true, visible = true;

  bool ctrl_click;

#ifdef SOFTWARE_ROTATE_DISPLAY
  RotatePointer rotate_pointer;
  PixelSize physical_screen_size{0, 0};
#endif

public:
  /**
   * @param queue the #EventQueue that shall receive X11 events
   */
  X11EventQueue(Display &display, EventQueue &queue);

  bool IsVisible() const {
    return mapped && visible;
  }

  /**
   * Was the Ctrl key down during the last MOUSE_DOWN event?
   *
   * TODO: this is a kludge, and we should move this flag into an
   * event struct passed to Window::OnMouseDown().q
   */
  bool WasCtrlClick() const {
    return ctrl_click;
  }

#ifdef SOFTWARE_ROTATE_DISPLAY
  void SetScreenSize(PixelSize screen_size) noexcept;
  void SetDisplayOrientation(DisplayOrientation orientation) noexcept;
#endif

  bool Generate([[maybe_unused]] Event &event) {
    return false;
  }

private:
  void HandleEvent(_XEvent &event);

  void OnSocketReady(unsigned events) noexcept;
};

} // namespace UI
