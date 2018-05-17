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

#include "FAITrianglePointValidator.hpp"
#include "FAITriangleRules.hpp"
#include "../Ordered/OrderedTask.hpp"
#include "../Ordered/Points/OrderedTaskPoint.hpp"
#include "Waypoint/Waypoint.hpp"

/** min distance for any FAI Leg -- derived from circular FAI sector radius */
static constexpr double min_fai_leg(2000);

/** min angle allowable in a FAI Triangle 31.5 degrees */
static constexpr Angle min_fai_angle = Angle::Degrees(31.5);

/** max angle allowable in a FAI Triangle 113.2 degrees */
static constexpr Angle max_fai_angle = Angle::Degrees(114);

FAITrianglePointValidator::FAITrianglePointValidator(
    OrderedTask *ordered_task, const unsigned ordered_task_index)
  :task(ordered_task), t_index(ordered_task_index), t_size(0)
{
  if (task != nullptr) {
    t_size = task->TaskSize();
    leg1 = t_size > 1
      ? task->GetTaskPoint(1).GetVectorPlanned().distance : 0;
    leg2 = t_size > 2
      ? task->GetTaskPoint(2).GetVectorPlanned().distance : 0;
    leg3 = t_size > 3
      ? task->GetTaskPoint(3).GetVectorPlanned().distance : 0;
  }

  fai_triangle_point_invalid = t_size > 4 || t_index > 3;
}


inline bool
FAITrianglePointValidator::TestFAITriangle(const double d1, const double d2,
                                           const double d3,
                                           const FAITriangleSettings &settings)
{
  if ((d1 < min_fai_leg) || (d2 < min_fai_leg) || (d3 < min_fai_leg))
    return false;

  return FAITriangleRules::TestDistances(d1, d2, d3, settings);
}

inline bool
FAITrianglePointValidator::IsFAIAngle(const GeoPoint &p0, const GeoPoint &p1,
                                      const GeoPoint &p2, bool right)
{
  const Angle a01 = p0.Bearing(p1);
  const Angle a21 = p2.Bearing(p1);
  const Angle diff = (a01 - a21).AsDelta();

  if (right)
    return (diff > min_fai_angle) && (diff < max_fai_angle);
  else
    return diff < -min_fai_angle && diff > -max_fai_angle;
}

bool
FAITrianglePointValidator::IsFAITrianglePoint(const Waypoint& wp,
                                              bool right) const
{
  if (fai_triangle_point_invalid)
    return false;

  if (t_size == 0)
    return true;

  assert(task != nullptr);

  const FAITriangleSettings &settings =
    task->GetOrderedTaskSettings().fai_triangle;

  const GeoPoint &p = wp.location;
  // replace start
  if (t_index == 0) {
    assert(t_size <= 4 && t_size > 0);

    switch (t_size) {
    case 1:
      return true;

    case 2:
      return p.Distance(task->GetPoint(1).GetLocation()) > min_fai_leg;

    default: // size == 3 or 4
      if (!IsFAIAngle(p, task->GetPoint(1).GetLocation(),
                      task->GetPoint(2).GetLocation(), right))
        return false;
      if (t_size == 3) {
        return TestFAITriangle(p.Distance(task->GetPoint(1).GetLocation()),
                               leg2,
                               task->GetPoint(2).GetLocation().Distance(p),
                               settings);
      } else if (t_size == 4) {
        return (wp == task->GetPoint(3).GetWaypoint()) &&
               TestFAITriangle(p.Distance(task->GetPoint(1).GetLocation()),
                               leg2, leg3,
                               settings);
      }
    }
  }
  // append or replace point #1
  if (t_index == 1) {
    assert(t_size > 0);

    if (t_size <= 2)
      return p.Distance(task->GetPoint(0).GetLocation()) > min_fai_leg;

    // size == 3 or 4
    if (!IsFAIAngle(task->GetPoint(0).GetLocation(),
                    p,
                    task->GetPoint(2).GetLocation(), right))
      return false;

    if (t_size == 3) {
      return TestFAITriangle(p.Distance(task->GetPoint(0).GetLocation()),
                             p.Distance(task->GetPoint(2).GetLocation()),
                             task->GetPoint(2).GetLocation().
                             Distance(task->GetPoint(0).GetLocation()),
                             settings);
    } else if (t_size == 4) {
      return TestFAITriangle(p.Distance(task->GetPoint(0).GetLocation()),
                             p.Distance(task->GetPoint(2).GetLocation()),
                             leg3,
                             settings);
    }
  }
  // append or replace point #2
  if (t_index == 2) {
    assert(t_size >= 2);
    if (!IsFAIAngle(task->GetPoint(0).GetLocation(),
                    task->GetPoint(1).GetLocation(),
                    p, right))
      return false;

    if (t_size < 4) { // no finish point yet
      return TestFAITriangle(leg1,
                             p.Distance(task->GetPoint(1).GetLocation()),
                             p.Distance(task->GetPoint(0).GetLocation()),
                             settings);

    } else { // already finish point(#3) exists
      return task->GetPoint(0).GetWaypoint() == task->GetPoint(3).GetWaypoint() &&
        TestFAITriangle(leg1,
                        p.Distance(task->GetPoint(1).GetLocation()),
                        p.Distance(task->GetPoint(0).GetLocation()),
                        settings);
    }
  }
  // append or replace finish
  if (t_index == 3) {
    assert (t_size == 3 || t_size == 4);
    return (wp == task->GetPoint(0).GetWaypoint()) &&
            TestFAITriangle(leg1,
                            leg2,
                            p.Distance(task->GetPoint(2).GetLocation()),
                            settings);
  }
  return true;
}
