/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "X11Queue.hpp"
#include "Queue.hpp"
#include "../Shared/Event.hpp"

/* kludges to work around namespace collisions with X11 headers */

#define Font X11Font
#define Window X11Window
#define Display X11Display

#include <X11/Xlib.h>
#include <X11/Xutil.h> // for XLookupString()

#undef Font
#undef Window
#undef Display

#include <stdio.h>
#include <stdlib.h>

X11EventQueue::X11EventQueue(boost::asio::io_service &io_service, EventQueue &_queue)
  :queue(_queue),
   display(XOpenDisplay(nullptr)),
   asio(io_service)
{
  if (display == nullptr) {
    fprintf(stderr, "XOpenDisplay() failed\n");
    exit(EXIT_FAILURE);
  }

  wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);

  asio.assign(ConnectionNumber(display));
  AsyncRead();
}

X11EventQueue::~X11EventQueue()
{
  asio.cancel();
  XCloseDisplay(display);
}

inline void
X11EventQueue::HandleEvent(_XEvent &event)
{
  switch (event.type) {
  case KeymapNotify:
    XRefreshKeyboardMapping(&event.xmapping);
    break;

  case KeyPress:
    {
      const auto key_sym = XLookupKeysym(&event.xkey, 0);
      if (key_sym == NoSymbol)
        break;

      Event e(Event::KEY_DOWN, key_sym);

      // this is ISO-Latin-1 only; TODO: switch to XwcTextEscapement()
      char ch;
      KeySym keysym;
      int n = XLookupString(&event.xkey, &ch, 1, &keysym, nullptr);
      e.ch = n > 0 ? ch : 0;

      queue.Push(e);
    }
    break;

  case KeyRelease:
    {
      const auto key_sym = XLookupKeysym(&event.xkey, 0);
      if (key_sym == NoSymbol)
        break;

      Event e(Event::KEY_DOWN, key_sym);
      queue.Push(Event(Event::KEY_UP, key_sym));
    }
    break;

  case ButtonPress:
    switch (event.xbutton.button) {
    case Button1:
    case Button2:
    case Button3:
      ctrl_click = event.xbutton.state & ControlMask;
      queue.Push(Event(Event::MOUSE_DOWN,
                       PixelPoint(event.xbutton.x, event.xbutton.y)));
      break;

    case Button4:
    case Button5:
      /* mouse wheel */
      {
        Event e(Event::MOUSE_WHEEL,
                PixelPoint(event.xbutton.x, event.xbutton.y));
        e.param = event.xbutton.button == Button4 ? 1u : unsigned(-1);
        queue.Push(e);
      }
      break;
    }
    break;

  case ButtonRelease:
    switch (event.xbutton.button) {
    case Button1:
    case Button2:
    case Button3:
      queue.Push(Event(Event::MOUSE_UP,
                       PixelPoint(event.xbutton.x, event.xbutton.y)));
    }
    break;

  case MotionNotify:
    queue.Push(Event(Event::MOUSE_MOTION,
                     PixelPoint(event.xmotion.x, event.xmotion.y)));
    break;

  case ClientMessage:
    if ((Atom)event.xclient.data.l[0] == wm_delete_window)
      queue.Push(Event::CLOSE);
    break;

  case Expose:
      queue.Push(Event::EXPOSE);
      break;

  case ConfigureNotify:
    queue.Push(Event(Event::RESIZE,
                     PixelPoint(event.xconfigure.width,
                                event.xconfigure.height)));
    break;

  case VisibilityNotify:
    visible = event.xvisibility.state != VisibilityFullyObscured;
    break;

  case UnmapNotify:
    mapped = false;
    break;

  case MapNotify:
    mapped = true;
    break;
  }
}

void
X11EventQueue::OnReadReady(const boost::system::error_code &ec)
{
  if (ec)
    return;

  while(XPending(display)) {
    XEvent event;
    XNextEvent(display, &event);
    HandleEvent(event);
  }

  AsyncRead();
}
