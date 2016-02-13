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

#ifndef OBSERVATIONZONE_HPP
#define OBSERVATIONZONE_HPP

#include "Compiler.h"

#include <stdint.h>

struct GeoPoint;
class OZBoundary;

/**
 * Abstract class giving properties of a zone which is used to measure
 * transitions in/out of and interior/exterior checking.
 */
class ObservationZone
{
public:
  enum class Shape : uint8_t {
    LINE,
    CYLINDER,
    MAT_CYLINDER,
    SECTOR,
    FAI_SECTOR,
    DAEC_KEYHOLE,
    BGAFIXEDCOURSE,
    BGAENHANCEDOPTION,
    BGA_START,
    ANNULAR_SECTOR,
    SYMMETRIC_QUADRANT,
    CUSTOM_KEYHOLE,
  };

private:
  const Shape shape;

  const bool can_start_through_top;

protected:
  ObservationZone(Shape _shape, bool _can_start_through_top)
    :shape(_shape), can_start_through_top(_can_start_through_top) {}

public:
  virtual ~ObservationZone() {}

  ObservationZone(const ObservationZone &) = delete;
  ObservationZone &operator=(const ObservationZone &) = delete;

  Shape GetShape() const {
    return shape;
  }

  /** 
   * Check whether observer is within OZ
   *
   * @return True if reference point is inside sector
   */
  gcc_pure
  virtual bool IsInSector(const GeoPoint &location) const = 0;

  /**
   * If zone when used for start can trigger task start via vertical exit
   *
   * @return True if zone type can have a valid start through top
   */
  gcc_pure
  bool CanStartThroughTop() const {
    return can_start_through_top;
  }

  /**
   * Check transition constraints
   *
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if constraints are satisfied
   */
  gcc_pure
  virtual bool TransitionConstraint(const GeoPoint &location,
                                    const GeoPoint &last_location) const = 0;

  /**
   * Return an unordered list of boundary points for evaluation by the
   * class #TaskDijkstra.  It shall contain all corners, all middle
   * points of straight lines and approximations for arcs.
   *
   * Even though there is no specific ordering, the most important
   * points should be at the front of the list if possible, to avoid
   * rounding problems as much as possible; for example, the
   * LineSectorZone has its center point at the beginning, so
   * TaskDijkstra chooses the outer points only if there is a
   * measurable advantage.
   */
  gcc_pure
  virtual OZBoundary GetBoundary() const = 0;

  /**
   * Distance reduction for scoring when outside this OZ
   * (used because FAI cylinders, for example, have their
   *  radius subtracted from scored distances)
   *
   * @return Distance (m) to subtract from score
   */
  gcc_pure
  virtual double ScoreAdjustment() const = 0;
};

#endif
