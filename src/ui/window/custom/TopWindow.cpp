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

#include "../TopWindow.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/Queue.hpp"
#include "ui/event/Globals.hpp"
#include "Hardware/CPU.hpp"

#ifdef ANDROID
#include "ui/event/android/Loop.hpp"
#include "util/ScopeExit.hxx"
#elif defined(ENABLE_SDL)
#include "ui/event/sdl/Event.hpp"
#include "ui/event/sdl/Loop.hpp"
#else
#include "ui/event/poll/Loop.hpp"
#include "ui/event/shared/Event.hpp"
#endif

#ifdef DRAW_MOUSE_CURSOR
#include "Screen/Layout.hpp"
#endif

namespace UI {

TopWindow::~TopWindow() noexcept
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
  screen = nullptr;

#ifdef ENABLE_SDL
  screen = new TopCanvas(display, window);
#elif defined(USE_GLX)
  screen = new TopCanvas(display, x_window);
#elif defined(USE_X11)
  screen = new TopCanvas(display, x_window);
#elif defined(USE_WAYLAND)
  screen = new TopCanvas(display, native_window);
#elif defined(USE_VFB)
  screen = new TopCanvas(display, size);
#else
  screen = new TopCanvas(display);
#endif

#ifdef SOFTWARE_ROTATE_DISPLAY
  size = screen->SetDisplayOrientation(style.GetInitialOrientation());
#elif defined(USE_MEMORY_CANVAS)
  size = screen->GetSize();
#endif
  ContainerWindow::Create(nullptr, PixelRect{size}, style);
}

#ifdef SOFTWARE_ROTATE_DISPLAY

void
TopWindow::SetDisplayOrientation(DisplayOrientation orientation) noexcept
{
  assert(screen != nullptr);

  Resize(screen->SetDisplayOrientation(orientation));
}

#endif

void
TopWindow::CancelMode() noexcept
{
  OnCancelMode();
}

void
TopWindow::Invalidate() noexcept
{
  invalidated = true;
}

#ifdef DRAW_MOUSE_CURSOR

inline void
TopWindow::DrawMouseCursor(Canvas &canvas) noexcept
{
  const auto m = event_queue->GetMousePosition();
  const int shortDistance = Layout::Scale(cursor_size * 4);
  const int longDistance = Layout::Scale(cursor_size * 6);

  const BulkPixelPoint p[] = {
    { m.x, m.y },
    { m.x + shortDistance, m.y + shortDistance },
    { m.x, m.y + longDistance },
  };

  if (invert_cursor_colors) {
    canvas.SelectWhitePen(cursor_size);
    canvas.SelectBlackBrush();
  } else {
    canvas.SelectBlackPen(cursor_size);
    canvas.SelectWhiteBrush();
  }
  canvas.DrawTriangleFan(p, std::size(p));
}

#endif

void
TopWindow::Expose() noexcept
{
#ifdef HAVE_CPU_FREQUENCY
  const ScopeLockCPU cpu;
#endif

  if (auto canvas = screen->Lock(); canvas.IsDefined()) {
    OnPaint(canvas);

#ifdef DRAW_MOUSE_CURSOR
    DrawMouseCursor(canvas);
#endif

    screen->Unlock();
  }

  screen->Flip();
}

void
TopWindow::Refresh() noexcept
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
TopWindow::OnActivate() noexcept
{
  return false;
}

bool
TopWindow::OnDeactivate() noexcept
{
  return false;
}

bool
TopWindow::OnClose() noexcept
{
  Destroy();
  return true;
}

int
TopWindow::RunEventLoop() noexcept
{
#ifdef ANDROID
  BeginRunning();
  AtScopeExit(this) { EndRunning(); };
#endif

  Refresh();

  EventLoop loop(*event_queue, *this);
  Event event;
  while (IsDefined() && loop.Get(event))
    loop.Dispatch(event);

  return 0;
}

void
TopWindow::PostQuit() noexcept
{
  event_queue->Quit();
}

} // namespace UI
