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
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/shared/Event.hpp"
#include "ui/event/poll/Loop.hpp"
#include "ui/event/Queue.hpp"
#include "ui/event/Globals.hpp"

#ifdef DRAW_MOUSE_CURSOR
#include "util/Macros.hpp"
#include "Screen/Layout.hpp"
#endif

#ifdef KOBO
#include "ui/canvas/Canvas.hpp"
#endif

#ifdef USE_FB
#include "ui/canvas/memory/Canvas.hpp"
#endif

namespace UI {

#ifdef USE_FB

void
TopWindow::CheckResize() noexcept
{
  assert(screen != nullptr);

  if (screen->CheckResize())
    Resize(screen->GetSize());
}

#endif

void
TopWindow::Invalidate() noexcept
{
  invalidated = true;
}

#ifdef KOBO
void
TopWindow::OnDestroy()
{
  /* clear the screen before exiting XCSoar */
  Canvas canvas = screen->Lock();
  if (canvas.IsDefined()) {
    canvas.Clear(COLOR_BLACK);
    screen->Flip();
    screen->Wait();

    canvas.ClearWhite();
    screen->Unlock();
    screen->Flip();
  }

  ContainerWindow::OnDestroy();
}
#endif

void
TopWindow::OnResize(PixelSize new_size)
{
  event_queue->SetScreenSize(new_size.width, new_size.height);

  ContainerWindow::OnResize(new_size);
}

#ifdef DRAW_MOUSE_CURSOR
void
TopWindow::OnPaint(Canvas &canvas)
{
  ContainerWindow::OnPaint(canvas);

  /* draw the mouse cursor */

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
  canvas.DrawTriangleFan(p, ARRAY_SIZE(p));
}
#endif

#ifndef NON_INTERACTIVE

bool
TopWindow::OnEvent(const Event &event)
{
  switch (event.type) {
    Window *w;

  case Event::NOP:
  case Event::CALLBACK:
    break;

  case Event::CLOSE:
    OnClose();
    break;

  case Event::KEY_DOWN:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    return w->OnKeyDown(event.param);

  case Event::KEY_UP:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    return w->OnKeyUp(event.param);

  case Event::MOUSE_MOTION:
#ifdef DRAW_MOUSE_CURSOR
    /* redraw to update the mouse cursor position */
    Invalidate();
#endif

    // XXX keys
    return OnMouseMove(event.point, 0);

  case Event::MOUSE_DOWN:
    return double_click.Check(event.point)
      ? OnMouseDouble(event.point)
      : OnMouseDown(event.point);

  case Event::MOUSE_UP:
    double_click.Moved(event.point);

    return OnMouseUp(event.point);

  case Event::MOUSE_WHEEL:
    return OnMouseWheel(event.point, (int)event.param);

#ifdef USE_X11
  case Event::RESIZE:
    if (screen->CheckResize(PixelSize(event.point.x, event.point.y)))
      Resize(screen->GetSize());
    return true;

  case Event::EXPOSE:
    Invalidate();
    return true;
#endif
  }

  return false;
}

#endif

int
TopWindow::RunEventLoop() noexcept
{
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
