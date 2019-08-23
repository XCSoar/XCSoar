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

#ifndef XCSOAR_TIMEOUT_CLOCK_HPP
#define XCSOAR_TIMEOUT_CLOCK_HPP

#include "PeriodClock.hpp"
#include "Util/Compiler.h"

#include <algorithm>

/**
 * A clock that can be used to check whether a timeout has expired.
 */
class TimeoutClock : private PeriodClock {
public:
  template<class Rep, class Period>
  explicit TimeoutClock(const std::chrono::duration<Rep,Period> &max_duration) noexcept {
    UpdateWithOffset(max_duration);
  }

  gcc_pure
  bool HasExpired() const {
    return Elapsed() > Duration::zero();
  }

  /**
   * Returns the number of milliseconds remaining until the timeout
   * expires.  The time has already expired if the return value is
   * negative.
   */
  gcc_pure
  std::chrono::steady_clock::duration GetRemainingSigned() const {
    return -Elapsed();
  }

  /**
   * Returns the number of milliseconds remaining until the timeout
   * expires.  The time has already expired if the return value is
   * 0.
   */
  gcc_pure
  std::chrono::steady_clock::duration GetRemainingOrZero() const {
    return std::max(GetRemainingSigned(),
                    std::chrono::steady_clock::duration::zero());
  }
};

#endif
