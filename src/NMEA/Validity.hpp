/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Math/fixed.hpp"

/**
 * This keeps track when a value was last changed, to check if it was
 * updated recently or to see if it has expired.  Additionally, it can
 * track if the attribute is not set (timestamp is zero).
 */
class Validity {
  fixed last;

public:
  /**
   * Cheap default constructor without initialization.
   */
  Validity() {}

  /**
   * Initialize the object with the specified timestamp.
   */
  Validity(fixed _last):last(_last) {}

public:
  /**
   * Clears the time stamp, marking the referenced value "invalid".
   */
  void Clear() {
    last = fixed_zero;
  }

  /**
   * Updates the time stamp, setting it to the current clock.  This
   * marks the referenced value as "valid".
   *
   * @param now the current time stamp in seconds
   */
  void Update(fixed now) {
    last = now;
  }

  /**
   * Checks if the time stamp has expired, and calls clear() if so.
   *
   * @param now the current time stamp in seconds
   * @param max_age the maximum age in seconds
   */
  void Expire(fixed now, fixed max_age) {
    if (positive(last) &&
        (now < last || /* time warp? */
         now > last + max_age)) /* expired? */
      Clear();
  }

  bool IsValid() const {
    return positive(last);
  }

  /**
   * This function calculates the time difference of the two Validity objects
   * @param other The second Validity object
   * @return The time difference in seconds
   */
  fixed GetTimeDifference(const Validity &other) const {
    assert(positive(last));
    assert(positive(other.last));

    return last - other.last;
  }

  /**
   * Was the value modified since the time the "other" object was
   * taken?
   */
  bool Modified(const Validity &other) const {
    return last > other.last;
  }

  bool operator==(const Validity &other) const {
    return last == other.last;
  }

  bool operator!=(const Validity &other) const {
    return last != other.last;
  }

  bool Complement(const Validity &other) {
    if (!IsValid() && other.IsValid()) {
      *this = other;
      return true;
    } else
      return false;
  }

  operator bool() const {
    return IsValid();
  }
};

#endif
