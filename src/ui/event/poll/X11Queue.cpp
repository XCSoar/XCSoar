// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef ENABLE_OPENGL
#include "ui/opengl/Features.hpp"
#endif
#include "X11Queue.hpp"
#include "Queue.hpp"
#include "../shared/Event.hpp"
#include "ui/display/Display.hpp"
#include "ui/dim/Size.hpp"

#if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
#include "../shared/TransformCoordinates.hpp"
#endif

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

PixelPoint
X11EventQueue::MaybeTransformPoint(PixelPoint p) const noexcept
{
#if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
  return TransformCoordinates(p, physical_screen_size);
#else
  return p;
#endif
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
      pointer_position = MaybeTransformPoint(
        PixelPoint(event.xbutton.x, event.xbutton.y));
      ctrl_click = event.xbutton.state & ControlMask;
      queue.Push(Event(Event::MOUSE_DOWN,
                       pointer_position));
      break;

    case Button4:
    case Button5:
      /* mouse wheel */
      {
        pointer_position = MaybeTransformPoint(
          PixelPoint(event.xbutton.x, event.xbutton.y));
        Event e(Event::MOUSE_WHEEL,
                pointer_position);
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
      pointer_position = MaybeTransformPoint(
        PixelPoint(event.xbutton.x, event.xbutton.y));
      queue.Push(Event(Event::MOUSE_UP,
                       pointer_position));
    }
    break;

  case MotionNotify:
    pointer_position = MaybeTransformPoint(
      PixelPoint(event.xmotion.x, event.xmotion.y));
    queue.Push(Event(Event::MOUSE_MOTION,
                     pointer_position));
    break;

  case ClientMessage:
    if ((Atom)event.xclient.data.l[0] == wm_delete_window)
      queue.Push(Event::CLOSE);
    break;

  case Expose:
      queue.Push(Event::EXPOSE);
      break;

  case ConfigureNotify:
    {
      if (event.xconfigure.width <= 0 || event.xconfigure.height <= 0)
        break;

      PixelSize physical_size(event.xconfigure.width,
                              event.xconfigure.height);
      physical_screen_size = physical_size;
      queue.Push(Event(Event::RESIZE,
                       PixelPoint(physical_size.width,
                                  physical_size.height)));
    }
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
  while (XPending(display)) {
    XEvent event;
    XNextEvent(display, &event);
    HandleEvent(event);
  }
}

} // namespace UI
