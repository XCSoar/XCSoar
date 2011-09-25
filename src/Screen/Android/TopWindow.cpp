/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Screen/OpenGL/Cache.hpp"
#include "Screen/OpenGL/Surface.hpp"
#include "Screen/OpenGL/Shapes.hpp"
#include "Screen/Android/Event.hpp"
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "PeriodClock.hpp"

void
TopWindow::AnnounceResize(UPixelScalar width, UPixelScalar height)
{
  ScopeLock protect(paused_mutex);
  resized = true;
  new_width = width;
  new_height = height;
}

void
TopWindow::RefreshSize()
{
  UPixelScalar width, height;

  {
    ScopeLock protect(paused_mutex);
    if (!resized)
      return;

    width = new_width;
    height = new_height;
  }

  resize(width, height);
}

bool
TopWindow::on_resize(UPixelScalar width, UPixelScalar height)
{
  if (native_view != NULL) {
    native_view->SetSize(width, height);
    screen.OnResize(width, height);
  }

  ContainerWindow::on_resize(width, height);
  return true;
}

void
TopWindow::on_pause()
{
  if (paused)
    return;

  TextCache::flush();
  OpenGL::DeinitShapes();

  SurfaceDestroyed();

  native_view->deinitSurface();

  paused_mutex.Lock();
  paused = true;
  resumed = false;
  paused_cond.Signal();
  paused_mutex.Unlock();
}

void
TopWindow::on_resume()
{
  if (!paused)
    return;

  /* tell TopWindow::expose() to reinitialize OpenGL */
  resumed = true;

  /* schedule a redraw */
  invalidate();
}

static bool
match_pause_and_resume(const Event &event, void *ctx)
{
  return event.type == Event::PAUSE || event.type == Event::RESUME;
}

void
TopWindow::pause()
{
  surface_valid = false;

  event_queue->purge(match_pause_and_resume, NULL);
  event_queue->push(Event::PAUSE);

  paused_mutex.Lock();
  while (!paused)
    paused_cond.Wait(paused_mutex);
  paused_mutex.Unlock();
}

void
TopWindow::resume()
{
  event_queue->purge(match_pause_and_resume, NULL);
  event_queue->push(Event::RESUME);
}

bool
TopWindow::on_event(const Event &event)
{
  switch (event.type) {
    Window *w;

  case Event::NOP:
  case Event::QUIT:
  case Event::TIMER:
  case Event::USER:
  case Event::NOTIFY:
    break;

  case Event::KEY_DOWN:
    w = get_focused_window();
    if (w == NULL)
      w = this;

    return w->on_key_down(event.param);

  case Event::KEY_UP:
    w = get_focused_window();
    if (w == NULL)
      w = this;

    return w->on_key_up(event.param);

  case Event::MOUSE_MOTION:
    // XXX keys
    return on_mouse_move(event.x, event.y, 0);

  case Event::MOUSE_DOWN:
    static PeriodClock double_click;
    return double_click.check_always_update(300)
      ? on_mouse_down(event.x, event.y)
      : on_mouse_double(event.x, event.y);

  case Event::MOUSE_UP:
    return on_mouse_up(event.x, event.y);

  case Event::RESIZE:
    if (!surface_valid)
      /* postpone the resize if we're paused; the real resize will be
         handled by TopWindow::refresh() as soon as XCSoar is
         resumed */
      return true;

    /* it seems the first page flip after a display orientation change
       is ignored on Android (tested on a Dell Streak / Android
       2.2.2); let's do one dummy call before we really draw
       something */
    screen.flip();

    resize(event.x, event.y);
    return true;

  case Event::PAUSE:
    on_pause();
    return true;

  case Event::RESUME:
    on_resume();
    return true;
  }

  return false;
}

int
TopWindow::event_loop()
{
  refresh();

  EventLoop loop(*event_queue, *this);
  Event event;
  while (defined() && loop.get(event))
    loop.dispatch(event);

  return 0;
}

void
TopWindow::post_quit()
{
  event_queue->push(Event::QUIT);
}
