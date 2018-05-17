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

#ifndef XCSOAR_ANGLE_RANGE_HPP
#define XCSOAR_ANGLE_RANGE_HPP

#include "Angle.hpp"

#include <type_traits>

/**
 * A range between two angles.  This class hosts a few algorithms that
 * are safe against wraparound.
 */
struct AngleRange {
  Angle start, end;

  AngleRange() = default;

  constexpr AngleRange(Angle _start, Angle _end)
    :start(_start), end(_end) {}

  constexpr bool IsEmpty() const {
    return start == end;
  }

  /**
   * Returns the length of the range.
   */
  gcc_pure
  Angle GetLength() const {
    return (end - start).AsBearing();
  }

  /**
   * Returns the middle of the range.  The return value is not
   * normalized.
   */
  gcc_pure
  Angle GetMiddle() const {
    return start.Fraction(end, 0.5);
  }

  /**
   * Is the specified value within the range?
   */
  gcc_pure
  bool IsInside(const Angle value) const {
    return value.Between(start, end);
  }

  /**
   * Do the two ranges overlap?
   */
  gcc_pure
  bool Overlaps(const AngleRange &other) const {
    return IsInside(other.start) || other.IsInside(start);
  }

  /**
   * Is the specified range within this range?
   */
  gcc_pure
  bool IsInside(const AngleRange &interior) const {
    return IsInside(interior.start) && IsInside(interior.end);
  }

  /**
   * Extend this range so the specified value is deemed inside.
   *
   * @return true if the range has been modified
   */
  bool Extend(Angle value);

  /**
   * Set this object to the intersection of this and the other object.
   *
   * @return false if the two objects do not overlap; in this case,
   * the object is left in an undefined state
   */
  bool IntersectWith(const AngleRange &other);
};

static_assert(std::is_trivial<AngleRange>::value, "type is not trivial");

#endif
