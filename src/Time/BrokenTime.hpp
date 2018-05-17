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

#ifndef XCSOAR_BROKEN_TIME_HPP
#define XCSOAR_BROKEN_TIME_HPP

#include "Compiler.h"

#include <type_traits>

#include <stdint.h>

/**
 * A broken-down representation of a time.
 */
struct BrokenTime {
  /**
   * Hour of day, 0-23.
   */
  uint8_t hour;

  /**
   * Minute, 0-59.
   */
  uint8_t minute;

  /**
   * Second, 0-59.
   */
  uint8_t second;

  /**
   * Non-initializing default constructor.
   */
  BrokenTime() = default;

  constexpr
  BrokenTime(unsigned _hour, unsigned _minute, unsigned _second=0)
    :hour(_hour), minute(_minute), second(_second) {}

  constexpr
  bool operator==(const BrokenTime other) const {
    return hour == other.hour && minute == other.minute &&
      second == other.second;
  }

  constexpr
  bool operator>(const BrokenTime other) const {
    return hour > other.hour ||
      (hour == other.hour && (minute > other.minute ||
                              (minute == other.minute && second > other.second)));
  }

  constexpr
  bool operator<(const BrokenTime other) const {
    return other > *this;
  }

  constexpr
  static BrokenTime Midnight() {
    return BrokenTime(0, 0);
  }

  /**
   * Returns an instance that fails the Plausible() check.
   */
  constexpr
  static BrokenTime Invalid() {
    return BrokenTime(24, 60, 60);
  }

  /**
   * Does this object contain plausible values?
   */
  constexpr
  bool IsPlausible() const {
    return hour < 24 && minute < 60 && second < 60;
  }

  /**
   * Returns the number of seconds which have passed on this day.
   */
  constexpr
  unsigned GetSecondOfDay() const {
    return hour * 3600u + minute * 60u + second;
  }

  /**
   * Construct a BrokenTime object from the specified number of
   * seconds which have passed on this day.
   *
   * @param second_of_day 0 .. 3600*24-1
   */
  gcc_const
  static BrokenTime FromSecondOfDay(unsigned second_of_day);

  /**
   * A wrapper for FromSecondOfDay() which allows values bigger than
   * or equal to 3600*24.
   */
  gcc_const
  static BrokenTime FromSecondOfDayChecked(unsigned second_of_day);

  /**
   * Returns the number of minutes which have passed on this day.
   */
  constexpr
  unsigned GetMinuteOfDay() const {
    return hour * 60u + minute;
  }

  /**
   * Construct a BrokenTime object from the specified number of
   * minutes which have passed on this day.
   *
   * @param minute_of_day 0 .. 60*24-1
   */
  gcc_const
  static BrokenTime FromMinuteOfDay(unsigned minute_of_day);

  /**
   * A wrapper for FromMinuteOfDay() which allows values bigger than
   * or equal to 60*24.
   */
  gcc_const
  static BrokenTime FromMinuteOfDayChecked(unsigned minute_of_day);

  /**
   * Returns a BrokenTime that has the specified number of seconds
   * added to it.  It properly wraps around midnight.
   *
   * @param seconds the number of seconds to add
   */
  gcc_pure
  BrokenTime operator+(unsigned seconds) const;

  /**
   * Returns a BrokenTime that has the specified number of seconds
   * added to it.  It properly wraps around midnight.
   *
   * @param seconds the number of seconds to add; may be negative
   */
  gcc_pure
  BrokenTime operator+(int seconds) const;

  gcc_pure
  BrokenTime operator-(int seconds) const {
    return *this + (-seconds);
  }

  gcc_pure
  BrokenTime operator-(unsigned seconds) const {
    return *this - int(seconds);
  }
};

static_assert(std::is_trivial<BrokenTime>::value, "type is not trivial");

#endif
