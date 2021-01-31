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

#ifndef SCANTASKPOINT_HPP
#define SCANTASKPOINT_HPP

#include <cstdint>

/**
 * A reference to a trace/search point: first element is the stage
 * number (turn point number); second element is the index in the
 * #TracePointVector / #SearchPointVector.
 */
class ScanTaskPoint {
  uint32_t value;

public:
  constexpr
  ScanTaskPoint(unsigned stage_number, unsigned point_index) noexcept
    :value((stage_number << 16) | point_index) {}

  /**
   * Generate a unique key that is used for the std::map comparison
   * operator.
   */
  constexpr
  uint32_t Key() const noexcept {
    return value;
  }

  constexpr
  bool operator==(const ScanTaskPoint other) const noexcept {
    return Key() == other.Key();
  }

  constexpr
  bool operator!=(const ScanTaskPoint other) const noexcept {
    return Key() != other.Key();
  }

  constexpr
  bool operator<(const ScanTaskPoint other) const noexcept {
    return Key() < other.Key();
  }

  constexpr
  unsigned GetStageNumber() const noexcept {
    return value >> 16;
  }

  constexpr
  unsigned GetPointIndex() const noexcept {
    return value & 0xffff;
  }

  void SetPointIndex(unsigned i) noexcept {
    value = (value & 0xffff0000) | i;
  }

  void IncrementPointIndex() noexcept {
    ++value;
  }

  /**
   * Determine whether a point is a starting point (no previous edges).
   */
  constexpr
  bool IsFirst() const noexcept {
    return GetStageNumber() == 0;
  }
};

#endif
