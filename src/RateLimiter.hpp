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

#ifndef XCSOAR_RATE_LIMITER_HPP
#define XCSOAR_RATE_LIMITER_HPP

#include "Event/Timer.hpp"
#include "Time/PeriodClock.hpp"

/**
 * A class that limits the rate at which events are processed.  It
 * postpones processing for a certain amount of time, and combines
 * several events.
 *
 * Due to its use of timers and the event queue, this class can only
 * be used in the main thread.
 */
class RateLimiter : private Timer {
  /**
   * Remember the last Run() invocation.
   */
  PeriodClock clock;

  unsigned period_ms, delay_ms;

public:
  /**
   * Constructor.
   *
   * @param period_ms the minimum duration between two invocations
   * @param delay_ms an event is delayed by this duration to combine
   * consecutive invocations
   */
  RateLimiter(unsigned _period_ms, unsigned _delay_ms=0);

  /**
   * Destructor.  Discards any pending events.
   */
  ~RateLimiter() {
    Cancel();
  }

  void Trigger();

  using Timer::Cancel;

protected:
  virtual void Run() = 0;

private:
  /* virtual methods from class Timer */
  virtual void OnTimer() override;
};

#endif
