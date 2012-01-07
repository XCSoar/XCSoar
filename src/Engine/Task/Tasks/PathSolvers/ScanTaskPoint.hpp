/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Compiler.h"

#include <stdint.h>

/**
 * A reference to a trace/search point: first element is the stage
 * number (turn point number); second element is the index in the
 * #TracePointVector / #SearchPointVector.
 */
struct ScanTaskPoint {
  uint16_t stage_number;

  uint16_t point_index;

  ScanTaskPoint(unsigned _stage_number, unsigned _point_index)
    :stage_number(_stage_number), point_index(_point_index) {}

  /**
   * Generate a unique key that is used for the std::map comparison
   * operator.
   */
  uint32_t Key() const {
    /* sorry for this dirty trick, but it speeds up the solver by 20%;
       language lawyers would say that this cast is not legal, but it
       works on all platforms we're interested in supporting */
    return *reinterpret_cast<const uint32_t *>(this);
  }

  bool operator<(const ScanTaskPoint &other) const {
    return Key() < other.Key();
  }

  /**
   * Determine whether a point is a starting point (no previous edges).
   */
  gcc_pure
  bool IsFirst() const {
    return stage_number == 0;
  }

} gcc_aligned(4); /* adjust alignment so the trick in Key() works on ARM */

#endif
