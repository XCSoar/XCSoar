/* Copyright_License {

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

#ifndef TASKPROJECTION_H
#define TASKPROJECTION_H

#include "FlatProjection.hpp"
#include "Geo/GeoBounds.hpp"
#include "Compiler.h"

struct GeoPoint;

/**
 * Class for performing Lambert Conformal Conic projections from
 * ellipsoidal Earth points to and from projected points.  Has
 * converters for projected coordinates in integer and double types.
 *
 * Needs to be initialized with reset() before first use.
 */
class TaskProjection : public FlatProjection {
  GeoBounds bounds;

public:
#ifndef NDEBUG
  TaskProjection():bounds(GeoBounds::Invalid()) {}
#else
  TaskProjection() = default;
#endif

  explicit TaskProjection(const GeoBounds &bounds);

  /**
   * Reset search bounds
   *
   * @param ref Default value for initial search bounds
   */
  void Reset(const GeoPoint &ref);

  /**
   * Check a location against bounds and update them if outside.
   * This does not update the projection itself.
   *
   * @param ref Point to check against bounds
   * @return true if the bounds have been modified
   */
  bool Scan(const GeoPoint &ref) {
    return bounds.Extend(ref);
  }

  /**
   * Update projection.
   *
   * @return True if projection changed
   */
  bool Update();

  /** 
   * Calculate radius of points used in task projection
   * 
   * note: this is an approximation that should only
   * be used for rendering purposes
   *
   * @return Radius (m) from center to edge
   */
  gcc_pure
  double ApproxRadius() const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const TaskProjection& task_projection);
#endif
};

#endif
