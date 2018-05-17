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

#ifndef XCSOAR_BROKEN_DATE_TIME_HPP
#define XCSOAR_BROKEN_DATE_TIME_HPP

#include "BrokenDate.hpp"
#include "BrokenTime.hpp"
#include "Compiler.h"

#include <type_traits>

#include <stdint.h>

/**
 * A broken-down representation of date and time.
 */
struct BrokenDateTime : public BrokenDate, public BrokenTime {
  /**
   * Non-initializing default constructor.
   */
  BrokenDateTime() = default;

  constexpr
  BrokenDateTime(unsigned _year, unsigned _month, unsigned _day,
                 unsigned _hour, unsigned _minute, unsigned _second=0)
    :BrokenDate(_year, _month, _day), BrokenTime(_hour, _minute, _second) {}

  constexpr
  BrokenDateTime(unsigned _year, unsigned _month, unsigned _day)
    :BrokenDate(_year, _month, _day), BrokenTime(0, 0) {}

  constexpr
  BrokenDateTime(const BrokenDate &date, const BrokenTime &time)
    :BrokenDate(date), BrokenTime(time) {}

  constexpr
  bool operator==(const BrokenDateTime other) const {
    return (const BrokenDate &)*this == (const BrokenDate &)other &&
      (const BrokenTime &)*this == (const BrokenTime &)other;
  }

  /**
   * Returns an instance that fails the Plausible() check.
   */
  constexpr
  static BrokenDateTime Invalid() {
    return BrokenDateTime(BrokenDate::Invalid(), BrokenTime::Invalid());
  }

  /**
   * Does the "date" part of this object contain plausible values?
   */
  constexpr
  bool IsDatePlausible() const {
    return BrokenDate::IsPlausible();
  }

  /**
   * Does the "time" part of this object contain plausible values?
   */
  constexpr
  bool IsTimePlausible() const {
    return BrokenTime::IsPlausible();
  }

  /**
   * Does this object contain plausible values?
   */
  constexpr
  bool IsPlausible() const {
    return IsDatePlausible() && IsTimePlausible();
  }

  /**
   * Returns a new #BrokenDateTime with the same date at midnight.
   */
  constexpr
  BrokenDateTime AtMidnight() const {
    return BrokenDateTime(*this, BrokenTime::Midnight());
  }

#ifdef HAVE_POSIX
  /**
   * Convert a UNIX UTC time stamp (seconds since epoch) to a
   * BrokenDateTime object.
   */
  gcc_const
  static BrokenDateTime FromUnixTimeUTC(int64_t t);
#endif

  gcc_pure
  int64_t ToUnixTimeUTC() const;

  /**
   * Returns the current system date and time, in UTC.
   */
  gcc_pure
  static const BrokenDateTime NowUTC();

  /**
   * Returns the current system date and time, in the current time zone.
   */
  gcc_pure
  static const BrokenDateTime NowLocal();

  /**
   * Returns a BrokenDateTime that has the specified number of seconds
   * added to it.
   *
   * @param seconds the number of seconds to add; may be negative
   */
  gcc_pure
  BrokenDateTime operator+(int seconds) const;

  gcc_pure
  BrokenDateTime operator-(int seconds) const {
    return *this + (-seconds);
  }

  /**
   * Returns the number of seconds between the two BrokenDateTime structs.
   * The second one is subtracted from the first one.
   *
   * <now> - <old> = positive timespan since <old> in seconds
   */
  gcc_pure
  int operator-(const BrokenDateTime &other) const;
};

static_assert(std::is_trivial<BrokenDateTime>::value, "type is not trivial");

#endif
