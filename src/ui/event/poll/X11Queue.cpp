// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "X11Queue.hpp"
#include "Queue.hpp"
#include "../shared/Event.hpp"
#include "ui/display/Display.hpp"

/* kludges to work around namespace collisions with X11 headers */

#define Font X11Font
#define Window X11Window
#define Display X11Display

#include <X11/Xlib.h>
#include <X11/Xutil.h> // for XLookupString()

#undef Font
#undef Window
#undef Display

#include <stdexcept>

namespace UI {

X11EventQueue::X11EventQueue(Display &_display, EventQueue &_queue)
  :queue(_queue),
   display(_display.GetXDisplay()),
   socket_event(queue.GetEventLoop(), BIND_THIS_METHOD(OnSocketReady))
{
  wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);

  socket_event.Open(SocketDescriptor(ConnectionNumber(display)));
  socket_event.ScheduleRead();
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
X11EventQueue::OnSocketReady(unsigned) noexcept
{
  while(XPending(display)) {
    XEvent event;
    XNextEvent(display, &event);
    HandleEvent(event);
  }
}

} // namespace UI
