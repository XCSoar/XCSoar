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

#include "Screen/Features.hpp"
#include "Screen/TopWindow.hpp"
#include "Event/SDL/Event.hpp"
#include "Event/SDL/Loop.hpp"
#include "Event/Globals.hpp"
#include "Event/Queue.hpp"
#include "Screen/Custom/TopCanvas.hpp"
#include "Util/ConvertString.hpp"
#include "Util/UTF8.hpp"

#ifdef UNICODE
#include "Util/ConvertString.hpp"
#endif

#if defined(__MACOSX__) && __MACOSX__
#include <SDL_syswm.h>
#import <AppKit/AppKit.h>
#include <alloca.h>
#endif

gcc_const
static Uint32
MakeSDLFlags(bool full_screen, bool resizable)
{
  Uint32 flags = 0;

#ifdef ENABLE_OPENGL
  flags |= SDL_WINDOW_OPENGL;
#else /* !ENABLE_OPENGL */
  flags |= SDL_SWSURFACE;
#endif /* !ENABLE_OPENGL */

#if !defined(__MACOSX__) || !(__MACOSX__)
  if (full_screen)
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

  if (resizable)
    flags |= SDL_WINDOW_RESIZABLE;

#if defined(__IPHONEOS__) && __IPHONEOS__
  /* Hide status bar on iOS devices */
  flags |= SDL_WINDOW_BORDERLESS;
#endif

#ifdef HAVE_HIGHDPI_SUPPORT
  flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

  return flags;
}

void
TopWindow::CreateNative(const TCHAR *_text, PixelSize new_size,
                        TopWindowStyle style)
{
#ifdef UNICODE
  const WideToUTF8Converter text(_text);
#else
  const char *text = _text;
#endif

  const bool full_screen = style.GetFullScreen();
  const bool resizable = style.GetResizable();
  const Uint32 flags = MakeSDLFlags(full_screen, resizable);

  window = ::SDL_CreateWindow(text, SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, new_size.cx,
                              new_size.cy, flags);
  if (window == nullptr) {
    fprintf(stderr,
            "SDL_CreateWindow(%s, %u, %u, %u, %u, %#x) has failed: %s\n",
            text, (unsigned) SDL_WINDOWPOS_UNDEFINED,
            (unsigned) SDL_WINDOWPOS_UNDEFINED, (unsigned) new_size.cx,
            (unsigned) new_size.cy, (unsigned)flags,
            ::SDL_GetError());
    return;
  }

#if defined(__MACOSX__) && __MACOSX__
  SDL_SysWMinfo *wm_info =
      reinterpret_cast<SDL_SysWMinfo *>(alloca(sizeof(SDL_SysWMinfo)));
  SDL_VERSION(&wm_info->version);
  if ((SDL_GetWindowWMInfo(window, wm_info)) &&
      (wm_info->subsystem == SDL_SYSWM_COCOA)) {
    if (resizable) {
      [wm_info->info.cocoa.window
          setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
    }
    if (full_screen) {
      [wm_info->info.cocoa.window toggleFullScreen: nil];
    }
  }
#endif

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

  case SDL_KEYDOWN:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyDown(event.key.keysym.sym);

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
    return OnMouseMove(PointToReal(PixelPoint(event.motion.x, event.motion.y)),
                       0);

  case SDL_MOUSEBUTTONDOWN:
    {
      const auto p = PointToReal(PixelPoint(event.button.x, event.button.y));
      return double_click.Check(p)
        ? OnMouseDouble(p)
        : OnMouseDown(p);
    }

  case SDL_MOUSEBUTTONUP:
    {
      const auto p = PointToReal(PixelPoint(event.button.x, event.button.y));
      double_click.Moved(p);
      return OnMouseUp(p);
    }

  case SDL_QUIT:
    return OnClose();

  case SDL_MOUSEWHEEL:
    {
      PixelPoint p;
      SDL_GetMouseState(&p.x, &p.y);
#ifdef HAVE_HIGHDPI_SUPPORT
      p = PointToReal(p);
#endif
      return OnMouseWheel(p, event.wheel.y);
    }

  case SDL_WINDOWEVENT:
    switch (event.window.event) {

    case SDL_WINDOWEVENT_RESIZED:
#ifndef HAVE_HIGHDPI_SUPPORT
#ifdef ENABLE_OPENGL
      if (screen->CheckResize(PixelSize(event.window.data1, event.window.data2)))
        Resize(screen->GetSize());
#else
      Resize(event.window.data1, event.window.data2);
#endif
      return true;
#endif
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
#ifdef HAVE_HIGHDPI_SUPPORT
            int real_w, real_h;
            SDL_GL_GetDrawableSize(event_window, &real_w, &real_h);
            point_to_real_x = static_cast<float>(real_w) /
                              static_cast<float>(w);
            point_to_real_y = static_cast<float>(real_h) /
                              static_cast<float>(h);
            w = real_w;
            h = real_h;
#endif
#ifdef ENABLE_OPENGL
            if (screen->CheckResize(PixelSize(w, h)))
              Resize(screen->GetSize());
#else
            Resize(w, h);
#endif
          }

#if defined(__MACOSX__) && __MACOSX__
          SDL_SysWMinfo *wm_info =
              reinterpret_cast<SDL_SysWMinfo *>(alloca(sizeof(SDL_SysWMinfo)));
          SDL_VERSION(&wm_info->version);
          if ((SDL_GetWindowWMInfo(event_window, wm_info)) &&
              (wm_info->subsystem == SDL_SYSWM_COCOA)) {
            [wm_info->info.cocoa.window
                setCollectionBehavior:
                    NSWindowCollectionBehaviorFullScreenPrimary];
          }
          Invalidate();
#endif
        }
      }
      return true;

    case SDL_WINDOWEVENT_EXPOSED:
      invalidated = false;
      Expose();
      return true;
    }
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

#ifdef USE_MEMORY_CANVAS
  screen->OnResize(new_size);
#endif
}
