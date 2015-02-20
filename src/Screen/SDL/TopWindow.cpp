/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Screen/Features.hpp"
#include "Screen/TopWindow.hpp"
#include "Event/SDL/Event.hpp"
#include "Event/SDL/Loop.hpp"
#include "Event/Globals.hpp"
#include "Event/Queue.hpp"
#include "Screen/Custom/TopCanvas.hpp"
#include "Util/ConvertString.hpp"

#if SDL_MAJOR_VERSION >= 2
#include "Util/UTF8.hpp"
#endif

#if SDL_MAJOR_VERSION < 2
void
TopWindow::SetCaption(const TCHAR *caption)
{
  WideToUTF8Converter caption2(caption);
  if (caption2.IsValid())
    ::SDL_WM_SetCaption(caption2, nullptr);
}
#endif

void
TopWindow::Invalidate()
{
  invalidated = true;
}

bool
TopWindow::OnEvent(const SDL_Event &event)
{
  switch (event.type) {
    Window *w;

#if SDL_MAJOR_VERSION < 2
  case SDL_VIDEOEXPOSE:
    invalidated = false;
    Expose();
    return true;
#endif

  case SDL_KEYDOWN:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

#if SDL_MAJOR_VERSION >= 2
    return w->OnKeyDown(event.key.keysym.sym);
#else
    return w->OnKeyDown(event.key.keysym.sym) ||
      (event.key.keysym.unicode != 0 &&
       w->OnCharacter(event.key.keysym.unicode));
#endif

#if SDL_MAJOR_VERSION >= 2
  case SDL_TEXTINPUT:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

    if (*event.text.text) {
      std::pair<unsigned, const char *> next = NextUTF8(event.text.text);
      bool handled = w->OnCharacter(next.first);
      while (next.second) {
        next = NextUTF8(next.second);
        handled = w->OnCharacter(next.first) || handled;
      }
      return handled;
    } else
      return false;
#endif

  case SDL_KEYUP:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyUp(event.key.keysym.sym);

#ifdef HAVE_MULTI_TOUCH
  case SDL_FINGERDOWN:
    if (SDL_GetNumTouchFingers(event.tfinger.touchId) == 2)
      return OnMultiTouchDown();
    else
      return false;

  case SDL_FINGERUP:
    if (SDL_GetNumTouchFingers(event.tfinger.touchId) == 1)
      return OnMultiTouchUp();
    else
      return false;
#endif

  case SDL_MOUSEMOTION:
    // XXX keys
    return OnMouseMove(event.motion.x, event.motion.y, 0);

  case SDL_MOUSEBUTTONDOWN:
#if SDL_MAJOR_VERSION < 2
    if (event.button.button == SDL_BUTTON_WHEELUP)
      return OnMouseWheel(event.button.x, event.button.y, 1);
    else if (event.button.button == SDL_BUTTON_WHEELDOWN)
      return OnMouseWheel(event.button.x, event.button.y, -1);
#endif

    return double_click.Check(RasterPoint(event.button.x, event.button.y))
      ? OnMouseDouble(event.button.x, event.button.y)
      : OnMouseDown(event.button.x, event.button.y);

  case SDL_MOUSEBUTTONUP:
#if SDL_MAJOR_VERSION < 2
    if (event.button.button == SDL_BUTTON_WHEELUP ||
        event.button.button == SDL_BUTTON_WHEELDOWN)
      /* the wheel has already been handled in SDL_MOUSEBUTTONDOWN */
      return false;
#endif

    double_click.Moved(RasterPoint(event.button.x, event.button.y));

    return OnMouseUp(event.button.x, event.button.y);

  case SDL_QUIT:
    return OnClose();

#if SDL_MAJOR_VERSION < 2
  case SDL_VIDEORESIZE:
    Resize(event.resize.w, event.resize.h);
    return true;
#endif

#if SDL_MAJOR_VERSION >= 2
  case SDL_MOUSEWHEEL:
    int x, y;
    SDL_GetMouseState(&x, &y);
    return OnMouseWheel(x, y, event.wheel.y);

  case SDL_WINDOWEVENT:
    switch (event.window.event) {

    case SDL_WINDOWEVENT_RESIZED:
      Resize(event.window.data1, event.window.data2);
      return true;

    case SDL_WINDOWEVENT_RESTORED:
    case SDL_WINDOWEVENT_MOVED:
    case SDL_WINDOWEVENT_SHOWN:
    case SDL_WINDOWEVENT_MAXIMIZED:
      {
        SDL_Window* event_window = SDL_GetWindowFromID(event.window.windowID);
        if (event_window) {
          int w, h;
          SDL_GetWindowSize(event_window, &w, &h);
          if ((w >= 0) && (h >= 0)) {
            Resize(w, h);
          }
        }
      }
      return true;

    case SDL_WINDOWEVENT_EXPOSED:
      invalidated = false;
      Expose();
      return true;
    }
#endif
  }

  return false;
}

int
TopWindow::RunEventLoop()
{
  Refresh();

  EventLoop loop(*event_queue, *this);
  Event event;
  while (IsDefined() && loop.Get(event))
    loop.Dispatch(event);

  return 0;
}

void
TopWindow::PostQuit()
{
  event_queue->Quit();
}

void
TopWindow::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);

  screen->OnResize(new_size);
}

