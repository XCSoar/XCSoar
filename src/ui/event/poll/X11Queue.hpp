/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_EVENT_X11_EVENT_QUEUE_HPP
#define XCSOAR_EVENT_X11_EVENT_QUEUE_HPP

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
  explicit X11EventQueue(EventQueue &queue);

  ~X11EventQueue();

  _XDisplay *GetDisplay() const {
    return display;
  }

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

  bool Generate(Event &event) {
    return false;
  }

private:
  void HandleEvent(_XEvent &event);

  void OnSocketReady(unsigned events) noexcept;
};

} // namespace UI

#endif
