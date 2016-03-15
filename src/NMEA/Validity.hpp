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

#ifndef XCSOAR_VALIDITY_HPP
#define XCSOAR_VALIDITY_HPP

#include "Compiler.h"

#include <type_traits>

#include <assert.h>
#include <stdint.h>
#include <math.h>

/**
 * This keeps track when a value was last changed, to check if it was
 * updated recently or to see if it has expired.  Additionally, it can
 * track if the attribute is not set (timestamp is zero).
 */
class Validity {
  static constexpr int BITS = 6;

  uint32_t last;

  gcc_const
  static uint32_t Import(double time) {
#ifdef __BIONIC__
    /* ldexp() is utterly broken on Bionic, but ldexpf() works - which
       is good enough here */
    // https://code.google.com/p/android/issues/detail?id=203996
    return (uint32_t)ldexpf(time, BITS);
#else
    return (uint32_t)ldexp(time, BITS);
#endif
  }

  constexpr
  static uint32_t Import(unsigned time) {
    return (uint32_t)(time << BITS);
  }

  gcc_const
  static double Export(uint32_t i) {
#ifdef __BIONIC__
    /* ldexp() is utterly broken on Bionic, but ldexpf() works - which
       is good enough here */
    // https://code.google.com/p/android/issues/detail?id=203996
    return ldexpf(i, -BITS);
#else
    return ldexp(i, -BITS);
#endif
  }

public:
  /**
   * Cheap default constructor without initialization.
   */
  Validity() = default;

  /**
   * Initialize the object with the specified timestamp.
   */
  explicit Validity(double _last):last(Import(_last)) {}

public:
  /**
   * Clears the time stamp, marking the referenced value "invalid".
   */
  void Clear() {
    last = 0;
  }

  /**
   * Updates the time stamp, setting it to the current clock.  This
   * marks the referenced value as "valid".
   *
   * @param now the current time stamp in seconds
   */
  void Update(double now) {
    last = Import(now);
  }

  /**
   * Checks if the time stamp has expired, and calls clear() if so.
   *
   * @param now the current time stamp in seconds
   * @param max_age the maximum age in seconds
   * @return true if the value is expired
   */
  bool Expire(double _now, double _max_age) {
    const uint32_t now = Import(_now);
    const uint32_t max_age = Import(_max_age);

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
  bool IsOlderThan(double _now, double _max_age) const {
    if (!IsValid())
      return true;

    const uint32_t now = Import(_now);
    const uint32_t max_age = Import(_max_age);

    return (now < last || /* time warp? */
            now > last + max_age); /* expired? */
  }

  constexpr bool IsValid() const {
    return last > 0;
  }

  /**
   * This function calculates the time difference of the two Validity objects
   * @param other The second Validity object
   * @return The time difference in seconds
   */
  double GetTimeDifference(const Validity &other) const {
    assert(IsValid());
    assert(other.IsValid());

    return Export(last - other.last);
  }

  /**
   * Was the value modified since the time the "other" object was
   * taken?
   */
  constexpr bool Modified(const Validity &other) const {
    return last > other.last;
  }

  constexpr bool operator==(const Validity &other) const {
    return last == other.last;
  }

  constexpr bool operator!=(const Validity &other) const {
    return last != other.last;
  }

  bool Complement(const Validity &other) {
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
  bool FixTimeWarp(const Validity &current, unsigned max_period=300) {
    if (!IsValid())
      return false;

    if (last + Import(max_period) < current.last || last > current.last) {
      /* out of range, this is a time warp */
      Clear();
      return true;
    }

    return false;
  }

  constexpr operator bool() const {
    return IsValid();
  }
};

static_assert(std::is_trivial<Validity>::value, "type is not trivial");

#endif
