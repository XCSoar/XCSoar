// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  bool _acquire_surface = false;

  {
    const std::lock_guard lock{paused_mutex};
    resized = true;
    new_size = _new_size;

    /* this call originates from NativeView.surfaceChanged(), which
       implies that we have a surface */
    have_java_surface = true;

    if (!have_native_surface)
      _acquire_surface = should_acquire_surface = true;
  }

  if (_acquire_surface && event_queue != nullptr)
    event_queue->Inject(Event::SURFACE);
}

bool
TopWindow::ResumeSurface() noexcept
{
  /* Try to reinitialize OpenGL.  This often fails on the first
     attempt (IllegalArgumentException "Make sure the SurfaceView or
     associated SurfaceHolder has a valid Surface"), therefore we're
     trying again until we're successful. */

  assert(!screen->IsReady());

  {
    const std::lock_guard lock{paused_mutex};
    if (!have_java_surface || should_release_surface)
      return false;
  }

  try {
    if (!screen->AcquireSurface())
      return false;
  } catch (...) {
    LogError(std::current_exception(), "Failed to initialize GL surface");
    return false;
  }

  assert(screen->IsReady());

  RefreshSize();

  /* schedule a redraw */
  Invalidate();

  return true;
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

  if (!screen->IsReady()) {
    ResumeSurface();
    return;
  }

  Resize(new_size_copy);
}

inline void
TopWindow::OnSurface() noexcept
{
  bool _release_surface, _acquire_surface;

  {
    const std::lock_guard lock{paused_mutex};
    _release_surface = should_release_surface;
    _acquire_surface = should_acquire_surface;
  }

  if (_release_surface) {
    TextCache::Flush();

    screen->ReleaseSurface();

    const std::lock_guard lock{paused_mutex};
    have_native_surface = false;
    should_release_surface = false;
    paused_cond.notify_one();
  }

  if (_acquire_surface) {
    if (!screen->IsReady() && !ResumeSurface())
      return;

    const std::lock_guard lock{paused_mutex};
    have_native_surface = true;
    should_acquire_surface = false;
  }
}

void
TopWindow::InvokeSurfaceDestroyed() noexcept
{
  {
    const std::lock_guard lock{paused_mutex};
    have_java_surface = false;
    should_release_surface = true;
    should_acquire_surface = false;
  }

  if (event_queue == nullptr)
    return;

  event_queue->Inject(Event::SURFACE);

  std::unique_lock lock{paused_mutex};
  paused_cond.wait(lock, [this]{ return !running || !should_release_surface; });
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  if (!screen->IsReady())
    ResumeSurface();

  if (native_view != nullptr)
    native_view->SetSize(new_size.width, new_size.height);

  ContainerWindow::OnResize(new_size);
}

void
TopWindow::OnPause() noexcept
{
}

void
TopWindow::OnResume() noexcept
{
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

  case Event::LOOK:
    OnLook();
    return true;

  case Event::SURFACE:
    OnSurface();
    return true;

  case Event::TASK_RECEIVED:
    OnTaskReceived();
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
