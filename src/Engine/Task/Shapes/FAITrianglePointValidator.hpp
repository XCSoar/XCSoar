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

#ifndef XCSOAR_FAI_TRIANGLE_POINT_VALIDATOR_HPP
#define XCSOAR_FAI_TRIANGLE_POINT_VALIDATOR_HPP

#include "Compiler.h"

class OrderedTask;
struct GeoPoint;
struct Waypoint;
struct FAITriangleSettings;

/**
 * Helper class to filter waypoints in list based on whether the filter is set
 * to Right or Left FAI Triangle.
 */
class FAITrianglePointValidator
{
  OrderedTask *const task;
  const unsigned t_index;
  unsigned t_size;
  double leg1;
  double leg2;
  double leg3;

  bool fai_triangle_point_invalid;

public:
  FAITrianglePointValidator(OrderedTask *ordered_task,
                            const unsigned ordered_task_index);

  /** Test whether wp could be a point in an FAI triangle based on the other
   * points in the task and the current ordered task index
   * Tests angle ranges first to reduce more costly calculation of distances
   * @param wp Point being tested
   * @param right = 1 if triangle turns are to right, -1 if turns are to left
   * @return True if point would be valid in an FAI Triangle
   */
  gcc_pure
  bool IsFAITrianglePoint(const Waypoint &wp, bool right) const;

private:
  gcc_pure
  static bool TestFAITriangle(double d1, double d2, double d3,
                              const FAITriangleSettings &settings);

  /**
   * Perform fast check to exclude point as from further consideration
   * based on min/max possible values for any FAI triangle
   * @param p0 point 1 of angle
   * @param p1 point 2 of angle
   * @param p2 point 3 of angle
   * @param right.  = 1 if angle is for right triangle, -1 if left triangle.
   * @returns False if angle from three points is out of possible range for
   * an FAI triangle.
   */
  gcc_pure
  static bool IsFAIAngle(const GeoPoint &p0, const GeoPoint &p1,
                         const GeoPoint &p2, bool right);

  void PrepareFAITest(OrderedTask *ordered_task,
                      const unsigned ordered_task_index);
};

#endif
