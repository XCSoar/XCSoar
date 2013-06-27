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

#ifndef XCSOAR_EVENT_TIMER_HPP
#define XCSOAR_EVENT_TIMER_HPP

#if defined(ANDROID) || defined(USE_CONSOLE)
#include <atomic>
#elif defined(ENABLE_SDL)
#include <SDL_timer.h>
#include <atomic>
#else
#include "Screen/Window.hpp"
#include "Screen/Timer.hpp"
#endif

#include <assert.h>
#include <stddef.h>

/**
 * A timer that, once initialized, periodically calls OnTimer() after
 * a specified amount of time, until Cancel() gets called.
 *
 * Initially, this class does not schedule a timer.
 *
 * This class is not thread safe; all of the methods must be called
 * from the main thread.
 *
 * The class #WindowTimer is cheaper on WIN32; use it instead of this
 * class if you are implementing a #Window.
 */
class Timer
#ifdef USE_GDI
  : private Window, private WindowTimer
#endif
{
#if defined(ANDROID) || defined(USE_CONSOLE)
  std::atomic<bool> enabled, queued;
  unsigned ms;
#elif defined(ENABLE_SDL)
  SDL_TimerID id;

  /**
   * True when the timer event has been pushed to the event queue.
   * This is used to prevent duplicate items stacking on the event
   * queue.
   */
  std::atomic<bool> queued;
#endif

public:
  /**
   * Construct a Timer object that is not set initially.
   */
#if defined(ANDROID) || defined(USE_CONSOLE)
  Timer():enabled(false), queued(false) {}
#elif defined(ENABLE_SDL)
  Timer():id(NULL), queued(false) {}
#else
  Timer():WindowTimer(*(Window *)this) {
    Window::CreateMessageWindow();
  }
#endif

  Timer(const Timer &other) = delete;

protected:
  /**
   * The move constructor may only be used on inactive timers.  This
   * shall only be used by derived classes to pass inactive instances
   * around.
   */
  Timer(Timer &&other)
#ifdef USE_GDI
    :WindowTimer(*(Window *)this)
#endif
  {
    assert(!IsActive());
    assert(!other.IsActive());
  }

public:
  ~Timer() {
    /* timer must be cleaned up explicitly */
    assert(!IsActive());

#ifdef USE_CONSOLE
    assert(!queued.load(std::memory_order_relaxed));
    assert(!enabled.load(std::memory_order_relaxed));
#endif
  }

#ifdef USE_GDI
  /* inherit WindowTimer's methods */
  using WindowTimer::IsActive;
  using WindowTimer::Schedule;
  using WindowTimer::Cancel;
#else

  /**
   * Is the timer active, i.e. is it waiting for the current period to
   * end?
   */
  bool IsActive() const {
#if defined(ANDROID) || defined(USE_CONSOLE)
    return enabled.load(std::memory_order_relaxed);
#elif defined(ENABLE_SDL)
    return id != NULL;
#endif
  }

  /**
   * Schedule the timer.  Cancels the previous setting if there was
   * one.
   */
  void Schedule(unsigned ms);

  /**
   * Cancels the scheduled timer, if any.  This is safe to be called
   * while the timer is running.
   */
  void Cancel();

#endif /* !GDI */

protected:
  /**
   * This method gets called after the configured time has elapsed.
   * Implement it.
   */
  virtual void OnTimer() = 0;

#if defined(ANDROID) || defined(USE_CONSOLE)
public:
  void Invoke();
#elif defined(ENABLE_SDL)
private:
  void Invoke();
  static void Invoke(void *ctx);

  Uint32 Callback(Uint32 interval);
  static Uint32 Callback(Uint32 interval, void *param);
#else
private:
  /* virtual methods from class Window */
  virtual bool OnTimer(WindowTimer &timer) override;
#endif
};

#endif
