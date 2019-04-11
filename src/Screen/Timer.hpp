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

#ifndef XCSOAR_SCREEN_TIMER_HXX
#define XCSOAR_SCREEN_TIMER_HXX

#ifdef USE_WINUSER
#include <windows.h>
#else
#include "Event/Timer.hpp"
#endif

#include <chrono>

#include <assert.h>

class Window;

/**
 * A timer that, once initialized, periodically calls
 * Window::OnTimer() after a specified amount of time, until Cancel()
 * gets called.
 *
 * Initially, this class does not schedule a timer.  It is supposed to
 * be used as an attribute of a Window class that uses it, being
 * reused as often as desired.
 *
 * This class is not thread safe; all of the methods must be called
 * from the main thread.
 */
class WindowTimer
#ifndef USE_WINUSER
  : private Timer
#endif
{
  Window &window;
#ifdef USE_WINUSER
  UINT_PTR id = 0;
#endif

public:
  /**
   * Construct a Timer object that is not set initially.
   */
  explicit WindowTimer(Window &_window)
    :window(_window) {}

  WindowTimer(const WindowTimer &other) = delete;

  ~WindowTimer() {
    /* timer must be cleaned up explicitly */
    assert(!IsActive());
  }

  /**
   * Is the timer active, i.e. is it waiting for the current period to
   * end?
   */
  bool IsActive() const {
#ifdef USE_WINUSER
    return id != 0;
#else
    return Timer::IsActive();
#endif
  }

  bool operator==(const WindowTimer &other) const {
    return this == &other;
  }

  bool operator!=(const WindowTimer &other) const {
    return !(*this == other);
  }

#ifdef USE_WINUSER
  /**
   * Schedule the timer.  Cancels the previous setting if there was
   * one.
   */
  void Schedule(std::chrono::steady_clock::duration d);

  /**
   * Cancels the scheduled timer, if any.  This is safe to be called
   * while the timer is running.
   */
  void Cancel();
#else
  using Timer::Schedule;
  using Timer::Cancel;

protected:
  virtual void OnTimer();
#endif
};

#endif
