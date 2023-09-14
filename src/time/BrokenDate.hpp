// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

/**
 * @brief Creates BrokenDate from julian date
 * 
 * @param julian_date as integer (floored)
 * @return BrokenDate 
 */
  static BrokenDate FromJulianDate(uint32_t julian_date) noexcept;
};

static_assert(std::is_trivial<BrokenDate>::value, "type is not trivial");
