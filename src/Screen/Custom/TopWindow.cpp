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

#include "Screen/TopWindow.hpp"
#include "Screen/Custom/TopCanvas.hpp"
#include "Hardware/CPU.hpp"

#ifdef USE_MEMORY_CANVAS
#include "Screen/Memory/Canvas.hpp"
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

#if defined(USE_X11) || defined(USE_WAYLAND) || defined(ENABLE_SDL)
  CreateNative(text, size, style);
#endif

  delete screen;
  screen = new TopCanvas();

#ifdef ENABLE_SDL
  screen->Create(window, size);
#elif defined(USE_GLX)
  screen->Create(x_display, x_window, fb_cfg);
#elif defined(USE_X11)
  screen->Create(x_display, x_window);
#elif defined(USE_WAYLAND)
  screen->Create(native_display, native_window);
#else
  screen->Create(size, style.GetFullScreen(), style.GetResizable());
#endif

  if (!screen->IsDefined()) {
    delete screen;
    screen = nullptr;
    return;
  }

  ContainerWindow::Create(nullptr, screen->GetRect(), style);
}

#ifdef SOFTWARE_ROTATE_DISPLAY

void
TopWindow::SetDisplayOrientation(DisplayOrientation orientation)
{
  assert(screen != nullptr);
  assert(screen->IsDefined());

  screen->SetDisplayOrientation(orientation);
  Resize(screen->GetSize());
}

#endif

void
TopWindow::CancelMode()
{
  OnCancelMode();
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
