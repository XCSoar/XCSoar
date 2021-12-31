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

#ifndef XCSOAR_BROKEN_DATE_HPP
#define XCSOAR_BROKEN_DATE_HPP

#include <type_traits>

#include <cstdint>

/**
 * A broken-down representation of a date.
 */
struct BrokenDate {
  /**
   * Absolute year, e.g. "2010".
   */
  uint16_t year;

  /**
   * Month number, 1-12.
   */
  uint8_t month;

  /**
   * Day of month, 1-31.
   */
  uint8_t day;

  /**
   * Day of the week (0-6, 0: sunday).  -1 means the value has not
   * been determined.
   */
  int8_t day_of_week;

  /**
   * Non-initializing default constructor.
   */
  BrokenDate() noexcept = default;

  constexpr
  BrokenDate(unsigned _year, unsigned _month, unsigned _day) noexcept
    :year(_year), month(_month), day(_day), day_of_week(-1) {}

  constexpr
  bool operator==(const BrokenDate other) const noexcept {
    return year == other.year && month == other.month && day == other.day;
  }

  constexpr
  bool operator>(const BrokenDate other) const noexcept {
    return year > other.year ||
      (year == other.year && (month > other.month ||
                              (month == other.month && day > other.day)));
  }

  constexpr
  bool operator<(const BrokenDate other) const noexcept {
    return other > *this;
  }

  /**
   * Clears the object, to make the Plausible() check returns false.
   */
  void Clear() noexcept {
    year = 0;
  }

  /**
   * Returns an instance that fails the Plausible() check.
   */
  constexpr
  static BrokenDate Invalid() noexcept {
    return BrokenDate(0, 0, 0);
  }

  /**
   * Does this object contain plausible values?
   */
  constexpr
  bool IsPlausible() const noexcept {
    return year >= 1800 && year <= 2500 &&
      month >= 1 && month <= 12 &&
      day >= 1 && day <= 31;
  }

  /**
   * Returns the current system date in UTC.
   */
  [[gnu::pure]]
  static BrokenDate TodayUTC() noexcept;

  void IncrementDay() noexcept;

  void DecrementDay() noexcept;

  /**
   * Returns the number of calendar days that have passed since the
   * two #BrokenDate structs.  The result may be negative if #other is
   * bigger than #this.
   */
  [[gnu::pure]]
  int DaysSince(const BrokenDate &other) const noexcept;
};

static_assert(std::is_trivial<BrokenDate>::value, "type is not trivial");

#endif
