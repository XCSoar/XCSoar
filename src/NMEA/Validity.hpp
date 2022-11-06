/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "time/Stamp.hpp"

#include <chrono>
#include <type_traits>

#include <cassert>
#include <cstdint>

/**
 * This keeps track when a value was last changed, to check if it was
 * updated recently or to see if it has expired.  Additionally, it can
 * track if the attribute is not set (timestamp is zero).
 */
class Validity {
  static constexpr int BITS = 6;

  using Duration = std::chrono::duration<uint32_t, std::ratio<1, 1 << BITS>>;

  Duration last;

  static constexpr Duration Import(TimeStamp time) noexcept {
    return time.Cast<Duration>();
  }

  static constexpr TimeStamp Export(Duration i) noexcept {
    return TimeStamp{std::chrono::duration_cast<FloatDuration>(i)};
  }

public:
  /**
   * Cheap default constructor without initialization.
   */
  Validity() = default;

  /**
   * Initialize the object with the specified timestamp.
   */
  explicit constexpr Validity(TimeStamp _last) noexcept
    :last(Import(_last)) {}

public:
  /**
   * Clears the time stamp, marking the referenced value "invalid".
   */
  constexpr void Clear() noexcept {
    last = Duration::zero();
  }

  /**
   * Updates the time stamp, setting it to the current clock.  This
   * marks the referenced value as "valid".
   *
   * @param now the current time stamp in seconds
   */
  constexpr void Update(TimeStamp now) noexcept {
    last = Import(now);
  }

  /**
   * Cast this object to an integer.  This integer should only be used
   * for equality comparisons with other integers from this method,
   * e.g. to check if a redraw is needed.
   */
  constexpr auto ToInteger() const noexcept {
    return last.count();
  }

  /**
   * Checks if the time stamp has expired, and calls clear() if so.
   *
   * @param now the current time stamp in seconds
   * @param max_age the maximum age in seconds
   * @return true if the value is expired
   */
  constexpr bool Expire(TimeStamp _now,
                        std::chrono::steady_clock::duration _max_age) noexcept {
    const auto now = Import(_now);
    const auto max_age = std::chrono::duration_cast<Duration>(_max_age);

    if (IsValid() &&
        (now < last || /* time warp? */
         now > last + max_age)) { /* expired? */
      Clear();
      return true;
    } else
      /* not expired */
      return false;
  }

  /**
   * Checks if the time stamp is older than the given time.
   *
   * @param now the current time stamp in seconds
   * @param max_age the maximum age in seconds
   * @return true if the value is expired
   */
  constexpr bool IsOlderThan(TimeStamp _now,
                             std::chrono::steady_clock::duration _max_age) const noexcept {
    if (!IsValid())
      return true;

    const auto now = Import(_now);
    const auto max_age = std::chrono::duration_cast<Duration>(_max_age);

    return (now < last || /* time warp? */
            now > last + max_age); /* expired? */
  }

  constexpr bool IsValid() const {
    return last > Duration::zero();
  }

  /**
   * This function calculates the time difference of the two Validity objects
   * @param other The second Validity object
   * @return The time difference in seconds
   */
  constexpr FloatDuration GetTimeDifference(const Validity &other) const noexcept {
    assert(IsValid());
    assert(other.IsValid());

    return std::chrono::duration_cast<FloatDuration>(last - other.last);
  }

  /**
   * Was the value modified since the time the "other" object was
   * taken?
   */
  constexpr bool Modified(const Validity &other) const noexcept {
    return last > other.last;
  }

  constexpr bool operator==(const Validity &other) const noexcept {
    return last == other.last;
  }

  constexpr bool operator!=(const Validity &other) const noexcept {
    return last != other.last;
  }

  constexpr bool operator<(const Validity &other) const noexcept {
    return last < other.last;
  }

  constexpr bool Complement(const Validity &other) noexcept {
    if (!IsValid() && other.IsValid()) {
      *this = other;
      return true;
    } else
      return false;
  }

  /**
   * Check this stored Validity object for a time warp and clear if it
   * one has occurred.  If this object is invalid, it is not
   * considered a time warp, even if the current object is valid.
   *
   * @param current the current "real" time stamp
   * @param max_period if time in "current" has advanced more than
   * this number of seconds, consider this a time warp, too
   * @return true if a time warp has occurred and this object has been
   * cleared, false if this object is within range
   */
  constexpr bool FixTimeWarp(const Validity &current,
                             std::chrono::steady_clock::duration _max_period=std::chrono::minutes(5)) noexcept {
    if (!IsValid())
      return false;

    const auto max_period = std::chrono::duration_cast<Duration>(_max_period);

    if (last + max_period < current.last || last > current.last) {
      /* out of range, this is a time warp */
      Clear();
      return true;
    }

    return false;
  }

  constexpr operator bool() const noexcept {
    return IsValid();
  }
};

static_assert(std::is_trivial<Validity>::value, "type is not trivial");
