// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Angle.hpp"

#include <type_traits>

/**
 * A range between two angles.  This class hosts a few algorithms that
 * are safe against wraparound.
 */
struct AngleRange {
  Angle start, end;

  AngleRange() noexcept = default;

  constexpr AngleRange(Angle _start, Angle _end) noexcept
    :start(_start), end(_end) {}

  constexpr bool IsEmpty() const noexcept {
    return start == end;
  }

  /**
   * Returns the length of the range.
   */
  [[gnu::pure]]
  Angle GetLength() const noexcept {
    return (end - start).AsBearing();
  }

  /**
   * Returns the middle of the range.  The return value is not
   * normalized.
   */
  [[gnu::pure]]
  Angle GetMiddle() const noexcept {
    return start.Fraction(end, 0.5);
  }

  /**
   * Is the specified value within the range?
   */
  [[gnu::pure]]
  bool IsInside(const Angle value) const noexcept {
    return value.Between(start, end);
  }

  /**
   * Do the two ranges overlap?
   */
  [[gnu::pure]]
  bool Overlaps(const AngleRange &other) const noexcept {
    return IsInside(other.start) || other.IsInside(start);
  }

  /**
   * Is the specified range within this range?
   */
  [[gnu::pure]]
  bool IsInside(const AngleRange &interior) const noexcept {
    return IsInside(interior.start) && IsInside(interior.end);
  }

  /**
   * Extend this range so the specified value is deemed inside.
   *
   * @return true if the range has been modified
   */
  bool Extend(Angle value) noexcept;

  /**
   * Set this object to the intersection of this and the other object.
   *
   * @return false if the two objects do not overlap; in this case,
   * the object is left in an undefined state
   */
  bool IntersectWith(const AngleRange &other) noexcept;
};

static_assert(std::is_trivial<AngleRange>::value, "type is not trivial");
