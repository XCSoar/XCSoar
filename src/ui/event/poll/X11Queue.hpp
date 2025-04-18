// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "event/SocketEvent.hxx"

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

  bool Generate([[maybe_unused]] Event &event) {
    return false;
  }

private:
  void HandleEvent(_XEvent &event);

  void OnSocketReady(unsigned events) noexcept;
};

} // namespace UI
