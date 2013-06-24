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
#include "Screen/Custom/TopCanvas.hpp"
#include "Event/Shared/Event.hpp"
#include "Event/Console/Loop.hpp"
#include "Event/Console/Queue.hpp"
#include "Event/Console/Globals.hpp"

#ifdef DRAW_MOUSE_CURSOR
#include "Util/Macros.hpp"
#include "Screen/Layout.hpp"
#endif

#ifdef KOBO
#include "Screen/Memory/Canvas.hpp"
#endif

void
TopWindow::SetCaption(const TCHAR *caption)
{
}

void
TopWindow::Invalidate()
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
  event_queue->SetScreenSize(new_size.cx, new_size.cy);
  screen->OnResize(new_size);
  ContainerWindow::OnResize(new_size);
}

#ifdef DRAW_MOUSE_CURSOR
void
TopWindow::OnPaint(Canvas &canvas)
{
  ContainerWindow::OnPaint(canvas);

  /* draw the mouse cursor */

  const RasterPoint m = event_queue->GetMousePosition();
  const RasterPoint p[] = {
    { m.x, m.y },
    { m.x + Layout::Scale(4), m.y + Layout::Scale(4) },
    { m.x, m.y + Layout::Scale(6) },
  };

  canvas.SelectBlackPen();
  canvas.SelectWhiteBrush();
  canvas.DrawTriangleFan(p, ARRAY_SIZE(p));
}
#endif

bool
TopWindow::OnEvent(const Event &event)
{
  switch (event.type) {
    Window *w;

  case Event::NOP:
  case Event::QUIT:
  case Event::TIMER:
  case Event::USER:
  case Event::CALLBACK:
    break;

  case Event::CLOSE:
    OnClose();
    break;

  case Event::KEY_DOWN:
    w = GetFocusedWindow();
    if (w == NULL)
      w = this;

    return w->OnKeyDown(event.param);

  case Event::KEY_UP:
    w = GetFocusedWindow();
    if (w == NULL)
      w = this;

    return w->OnKeyUp(event.param);

  case Event::MOUSE_MOTION:
#ifdef DRAW_MOUSE_CURSOR
    /* redraw to update the mouse cursor position */
    Invalidate();
#endif

    // XXX keys
    return OnMouseMove(event.x, event.y, 0);

  case Event::MOUSE_DOWN:
    return double_click.Check(event.GetPoint())
      ? OnMouseDouble(event.x, event.y)
      : OnMouseDown(event.x, event.y);

  case Event::MOUSE_UP:
    double_click.Moved(event.GetPoint());

    return OnMouseUp(event.x, event.y);
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
  event_queue->Push(Event::QUIT);
}
