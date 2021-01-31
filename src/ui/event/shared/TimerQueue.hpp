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

#ifndef XCSOAR_EVENT_TIMER_QUEUE_HPP
#define XCSOAR_EVENT_TIMER_QUEUE_HPP

#include "util/Compiler.h"

#include <chrono>
#include <set>

#include <cstdint>

namespace UI {

class Timer;

class TimerQueue {
  struct TimerRecord {
    /**
     * Projected MonotonicClockUS() value when this timer is due.
     */
    std::chrono::steady_clock::time_point due;

    Timer *timer;

    constexpr TimerRecord(Timer &_timer,
                          std::chrono::steady_clock::time_point _due) noexcept
      :due(_due), timer(&_timer) {}

    TimerRecord(TimerRecord &&other) = default;
    TimerRecord(const TimerRecord &other) = delete;
    TimerRecord &operator=(const TimerRecord &other) = delete;

    bool operator<(const TimerRecord &other) const {
      return due < other.due;
    }

    bool IsDue(std::chrono::steady_clock::time_point now) const noexcept {
      return now >= due;
    }
  };

  std::multiset<TimerRecord> timers;

public:
  TimerQueue() = default;
  TimerQueue(const TimerQueue &other) = delete;
  TimerQueue &operator=(const TimerQueue &other) = delete;

  /**
   * Does the specified time stamp occur before the first scheduled
   * timer?  If this returns true, then adding a timer at the
   * specified time stamp will require adjusting the poll timeout,
   * i.e. the event thread must be woken up.
   */
  bool IsBefore(std::chrono::steady_clock::time_point t) const noexcept {
    return timers.empty() || t < timers.begin()->due;
  }

  /**
   * Returns the number of microseconds until the next timer expires,
   * 0 if at least one has already expired, -1 if there are no timers.
   *
   * Caller must lock a mutex.
   */
  gcc_pure
  std::chrono::steady_clock::duration GetTimeout(std::chrono::steady_clock::time_point now) const noexcept;

  /**
   * Returns the first timer that is due now, nullptr if no timer is
   * due.
   *
   * Caller must lock a mutex.
   */
  Timer *Pop(std::chrono::steady_clock::time_point now) noexcept;

  /**
   * Caller must lock a mutex.
   */
  void Add(Timer &timer, std::chrono::steady_clock::time_point due) noexcept;

  /**
   * Caller must lock a mutex.
   */
  void Cancel(Timer &timer);
};

} // namespace UI

#endif
