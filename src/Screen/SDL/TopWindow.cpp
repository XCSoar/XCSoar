/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifdef ANDROID
#include "Screen/Android/Event.hpp"
#include "Android/Main.hpp"
#else
#include "Screen/SDL/Event.hpp"
#include "Util/ConvertString.hpp"
#endif

TopWindow::TopWindow()
  :invalidated(true)
#ifdef ANDROID
  , paused(false), resumed(false), resized(false)
#endif
{
}

bool
TopWindow::find(const TCHAR *cls, const TCHAR *text)
{
  return false; // XXX
}

void
TopWindow::set(const TCHAR *cls, const TCHAR *text, PixelRect rc,
               TopWindowStyle style)
{
  const UPixelScalar width = rc.right - rc.left;
  const UPixelScalar height = rc.bottom - rc.top;

  screen.Set(width, height, style.GetFullScreen(), style.GetResizable());

  ContainerWindow::set(NULL, 0, 0, width, height, style);

#ifndef ANDROID
  WideToUTF8Converter text2(text);
  if (text2.IsValid())
    ::SDL_WM_SetCaption(text2, NULL);
#endif
}

void
TopWindow::CancelMode()
{
  OnCancelMode();
}

void
TopWindow::Fullscreen()
{
  screen.Fullscreen();
}

void
TopWindow::Invalidate()
{
  invalidated_lock.Lock();
  if (invalidated) {
    /* already invalidated, don't send the event twice */
    invalidated_lock.Unlock();
    return;
  }

  invalidated = true;
  invalidated_lock.Unlock();

  /* wake up the event loop */
#ifdef ANDROID
  event_queue->Push(Event::NOP);
#else
  /* note that SDL_NOEVENT is not documented, but since we just want
     to wake up without actually sending an event, I hope this works
     on all future SDL versions; if SDL_NOEVENT ever gets remove, I'll
     have to come up with something else */
  SDL_Event event;
  event.type = SDL_NOEVENT;
  ::SDL_PushEvent(&event);
#endif
}

void
TopWindow::Expose() {
  OnPaint(screen);
  screen.Flip();
}

void
TopWindow::Refresh()
{
#ifdef ANDROID
  if (!CheckResumeSurface())
    /* the application is paused/suspended, and we don't have an
       OpenGL surface - ignore all drawing requests */
    return;
#endif

  invalidated_lock.Lock();
  if (!invalidated) {
    invalidated_lock.Unlock();
    return;
  }

  invalidated = false;
  invalidated_lock.Unlock();

  Expose();
}

bool
TopWindow::OnActivate()
{
  return false;
}

bool
TopWindow::OnDeactivate()
{
  return false;
}

bool
TopWindow::OnClose()
{
  reset();
  return true;
}

#ifndef ANDROID

bool
TopWindow::OnEvent(const SDL_Event &event)
{
  switch (event.type) {
    Window *w;

  case SDL_VIDEOEXPOSE:
    invalidated_lock.Lock();
    invalidated = false;
    invalidated_lock.Unlock();

    Expose();
    return true;

  case SDL_KEYDOWN:
    w = GetFocusedWindow();
    if (w == NULL)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyDown(event.key.keysym.sym);

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

    return double_click.Check(RasterPoint{PixelScalar(event.button.x),
                                          PixelScalar(event.button.y)})
      ? OnMouseDouble(event.button.x, event.button.y)
      : OnMouseDown(event.button.x, event.button.y);

  case SDL_MOUSEBUTTONUP:
    if (event.button.button == SDL_BUTTON_WHEELUP ||
        event.button.button == SDL_BUTTON_WHEELDOWN)
      /* the wheel has already been handled in SDL_MOUSEBUTTONDOWN */
      return false;

    double_click.Moved(RasterPoint{PixelScalar(event.button.x),
                                   PixelScalar(event.button.y)});

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

  EventLoop loop(*this);
  SDL_Event event;
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
TopWindow::OnResize(UPixelScalar width, UPixelScalar height)
{
  ContainerWindow::OnResize(width, height);

  screen.OnResize(width, height);
}

#endif /* !ANDROID */
