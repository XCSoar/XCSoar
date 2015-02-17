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

#include "Screen/TopWindow.hpp"
#include "Screen/Custom/TopCanvas.hpp"
#include "Hardware/CPU.hpp"

#ifdef USE_X11
#include "Event/Globals.hpp"
#include "Event/Queue.hpp"
#endif

#ifdef USE_MEMORY_CANVAS
#include "Screen/Memory/Canvas.hpp"
#endif

#if defined(UNICODE) && SDL_MAJOR_VERSION >= 2
#include "Util/ConvertString.hpp"
#endif

TopWindow::~TopWindow()
{
  delete screen;
}

void
TopWindow::Create(const TCHAR *text, PixelSize size,
                  TopWindowStyle style)
{
  invalidated = true;

#ifdef USE_X11
  x_display = event_queue->GetDisplay();
  assert(x_display != nullptr);

  const X11Window x_root = DefaultRootWindow(x_display);
  if (x_root == 0) {
    fprintf(stderr, "DefaultRootWindow() failed\n");
    exit(EXIT_FAILURE);
  }

  XSetWindowAttributes swa;
  swa.event_mask = KeyPressMask | KeyReleaseMask |
    ButtonPressMask | ButtonReleaseMask |
    PointerMotionMask |
    VisibilityChangeMask |
    ExposureMask | StructureNotifyMask;

  x_window = XCreateWindow(x_display, x_root,
                           0, 0, size.cx, size.cy, 0,
                           CopyFromParent, InputOutput,
                           CopyFromParent, CWEventMask,
                           &swa);
  if (x_window == 0) {
    fprintf(stderr, "XCreateWindow() failed\n");
    exit(EXIT_FAILURE);
  }

  XMapWindow(x_display, x_window);
  XStoreName(x_display, x_window, text);

  /* receive "Close" button clicks from the window manager */
  auto wm_delete_window = XInternAtom(x_display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(x_display, x_window, &wm_delete_window, 1);
#endif

  delete screen;
  screen = new TopCanvas();

#if defined(ENABLE_SDL) && (SDL_MAJOR_VERSION >= 2)
#ifdef UNICODE
  const WideToUTF8Converter text2(text);
#else
  const char* text2 = text;
#endif
  screen->Create(text2, size, style.GetFullScreen(), style.GetResizable());
#elif defined(USE_X11)
  screen->Create(x_display, x_window);
#else
  screen->Create(size, style.GetFullScreen(), style.GetResizable());
#endif

  if (!screen->IsDefined()) {
    delete screen;
    screen = nullptr;
    return;
  }

  ContainerWindow::Create(nullptr, screen->GetRect(), style);

#if defined(ENABLE_SDL) && (SDL_MAJOR_VERSION < 2)
  SetCaption(text);
#endif
}

#ifdef SOFTWARE_ROTATE_DISPLAY

void
TopWindow::SetDisplayOrientation(DisplayOrientation orientation)
{
  assert(screen != nullptr);
  assert(screen->IsDefined());

  screen->SetDisplayOrientation(orientation);
  Resize(screen->GetWidth(), screen->GetHeight());
}

#endif

void
TopWindow::CancelMode()
{
  OnCancelMode();
}

void
TopWindow::Fullscreen()
{
  screen->Fullscreen();
}

void
TopWindow::Expose()
{
#ifdef HAVE_CPU_FREQUENCY
  const ScopeLockCPU cpu;
#endif

#ifdef USE_MEMORY_CANVAS
  Canvas canvas = screen->Lock();
  if (canvas.IsDefined()) {
    OnPaint(canvas);
    screen->Unlock();
  }
#else
  OnPaint(*screen);
#endif

  screen->Flip();
}

void
TopWindow::Refresh()
{
  if (!CheckResumeSurface())
    /* the application is paused/suspended, and we don't have an
       OpenGL surface - ignore all drawing requests */
    return;

#ifdef USE_X11
  if (!IsVisible())
    /* don't bother to invoke the renderer if we're not visible on the
       X11 display */
    return;
#endif

  if (!invalidated)
    return;

  invalidated = false;

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
  Destroy();
  return true;
}
