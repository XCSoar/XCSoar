/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/TopWindow.hpp"
#include "Event/SDL/Event.hpp"
#include "Event/SDL/Loop.hpp"
#include "Event/Globals.hpp"
#include "Screen/Custom/TopCanvas.hpp"
#include "Util/ConvertString.hpp"

void
TopWindow::SetCaption(const TCHAR *caption)
{
  WideToUTF8Converter caption2(caption);
  if (caption2.IsValid())
    ::SDL_WM_SetCaption(caption2, nullptr);
}

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

  case SDL_VIDEOEXPOSE:
    invalidated = false;
    Expose();
    return true;

  case SDL_KEYDOWN:
    w = GetFocusedWindow();
    if (w == NULL)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyDown(event.key.keysym.sym) ||
      (event.key.keysym.unicode != 0 &&
       w->OnCharacter(event.key.keysym.unicode));

  case SDL_KEYUP:
    w = GetFocusedWindow();
    if (w == NULL)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyUp(event.key.keysym.sym);

  case SDL_MOUSEMOTION:
    // XXX keys
    return OnMouseMove(event.motion.x, event.motion.y, 0);

  case SDL_MOUSEBUTTONDOWN:
    if (event.button.button == SDL_BUTTON_WHEELUP)
      return OnMouseWheel(event.button.x, event.button.y, 1);
    else if (event.button.button == SDL_BUTTON_WHEELDOWN)
      return OnMouseWheel(event.button.x, event.button.y, -1);

    return double_click.Check(RasterPoint(event.button.x, event.button.y))
      ? OnMouseDouble(event.button.x, event.button.y)
      : OnMouseDown(event.button.x, event.button.y);

  case SDL_MOUSEBUTTONUP:
    if (event.button.button == SDL_BUTTON_WHEELUP ||
        event.button.button == SDL_BUTTON_WHEELDOWN)
      /* the wheel has already been handled in SDL_MOUSEBUTTONDOWN */
      return false;

    double_click.Moved(RasterPoint(event.button.x, event.button.y));

    return OnMouseUp(event.button.x, event.button.y);

  case SDL_QUIT:
    return OnClose();

  case SDL_VIDEORESIZE:
    Resize(event.resize.w, event.resize.h);
    return true;
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
  SDL_Event event;
  event.type = SDL_QUIT;
  ::SDL_PushEvent(&event);
}

void
TopWindow::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);

  screen->OnResize(new_size);
}

