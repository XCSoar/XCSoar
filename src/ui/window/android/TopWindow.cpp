/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
#include "ui/canvas/custom/Cache.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/Queue.hpp"
#include "ui/event/Globals.hpp"
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "LogFile.hpp"

#include <cassert>

namespace UI {

TopWindow::TopWindow(UI::Display &_display) noexcept
  :display(_display)
{
  native_view->SetPointer(Java::GetEnv(), this);
}

void
TopWindow::AnnounceResize(PixelSize _new_size) noexcept
{
  const std::lock_guard lock{paused_mutex};
  resized = true;
  new_size = _new_size;
}

bool
TopWindow::ResumeSurface() noexcept
{
  /* Try to reinitialize OpenGL.  This often fails on the first
     attempt (IllegalArgumentException "Make sure the SurfaceView or
     associated SurfaceHolder has a valid Surface"), therefore we're
     trying again until we're successful. */

  assert(paused);

  try {
    if (!screen->AcquireSurface())
      return false;
  } catch (...) {
    LogError(std::current_exception(), "Failed to initialize GL surface");
    return false;
  }

  assert(screen->IsReady());

  paused = false;
  resumed = false;

  RefreshSize();

  return true;
}

bool
TopWindow::CheckResumeSurface() noexcept
{
  return (!resumed || ResumeSurface()) && !paused && screen->IsReady();
}

void
TopWindow::RefreshSize() noexcept
{
  PixelSize new_size_copy;

  {
    const std::lock_guard lock{paused_mutex};
    if (!resized)
      return;

    resized = false;
    new_size_copy = new_size;
  }

  if (screen->CheckResize(new_size_copy))
    Resize(new_size_copy);
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  if (native_view != nullptr)
    native_view->SetSize(new_size.width, new_size.height);

  ContainerWindow::OnResize(new_size);
}

void
TopWindow::OnPause() noexcept
{
  if (paused)
    return;

  TextCache::Flush();

  screen->ReleaseSurface();

  assert(!screen->IsReady());

  const std::lock_guard lock{paused_mutex};
  paused = true;
  resumed = false;
  paused_cond.notify_one();
}

void
TopWindow::OnResume() noexcept
{
  if (!paused)
    return;

  /* tell TopWindow::Expose() to reinitialize OpenGL */
  resumed = true;

  /* schedule a redraw */
  Invalidate();
}

static bool
match_pause_and_resume(const Event &event, [[maybe_unused]] void *ctx) noexcept
{
  return event.type == Event::PAUSE || event.type == Event::RESUME;
}

void
TopWindow::Pause() noexcept
{
  event_queue->Purge(match_pause_and_resume, nullptr);
  event_queue->Inject(Event::PAUSE);

  std::unique_lock lock{paused_mutex};
  paused_cond.wait(lock, [this]{ return !running || paused; });
}

void
TopWindow::Resume() noexcept
{
  event_queue->Purge(match_pause_and_resume, nullptr);
  event_queue->Inject(Event::RESUME);
}

bool
TopWindow::OnEvent(const Event &event)
{
  switch (event.type) {
    Window *w;

  case Event::NOP:
  case Event::TIMER:
  case Event::CALLBACK:
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

  case Event::POINTER_DOWN:
    return OnMultiTouchDown();

  case Event::POINTER_UP:
    return OnMultiTouchUp();

  case Event::RESIZE:
    if (!screen->IsReady())
      /* postpone the resize if we're paused; the real resize will be
         handled by TopWindow::refresh() as soon as XCSoar is
         resumed */
      return true;

    if (screen->CheckResize(PixelSize(event.point.x, event.point.y)))
      Resize(screen->GetSize());

    /* it seems the first page flip after a display orientation change
       is ignored on Android (tested on a Dell Streak / Android
       2.2.2); let's do one dummy call before we really draw
       something */
    screen->Flip();
    return true;

  case Event::PAUSE:
    OnPause();
    return true;

  case Event::RESUME:
    OnResume();
    return true;
  }

  return false;
}

void
TopWindow::BeginRunning() noexcept
{
  const std::lock_guard lock{paused_mutex};
  assert(!running);
  running = true;
}

void
TopWindow::EndRunning() noexcept
{
  const std::lock_guard lock{paused_mutex};
  assert(running);
  running = false;
  /* wake up the Android Activity thread, just in case it's waiting
     inside Pause() */
  paused_cond.notify_one();
}

} // namespace UI
