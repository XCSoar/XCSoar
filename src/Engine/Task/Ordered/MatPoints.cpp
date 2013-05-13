/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "MatPoints.hpp"
#include "Waypoint/WaypointVisitor.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Points/OrderedTaskPoint.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Task/Ordered/Points/IntermediatePoint.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/fixed.hpp"

#include <set>

/**
 * Class to build vector from visited waypoints.
 * Intended to be used temporarily.
 */
class MatWaypointVisitorVector final : public WaypointVisitor {
public:
  static constexpr unsigned MAX_MAT_POINTS = 256;

  /**
   * range in km of max distance from center of task
   * (used for large waypoint files to limit MatPoints size)
   */
  static constexpr unsigned OPTIONAL_RANGE_LIMIT = 160;

private:
  MatPoints::MatVector &vector;

  /**
   * a temp map used to dedupe mat points when
   * we're adding via closest first
   */
  std::set<unsigned> dedupe;

  const AbstractTaskFactory& factory;

  /**
   * adds to the vector if it is not already in the vector
   * assumes vector has only been added to via this function
   * @param wp.  wp of the TP to be added
   */
  void AddUnique(const Waypoint &wp) {
    const unsigned key = wp.id;

    if (dedupe.insert(key).second) {
      OrderedTaskPoint *tp = factory.CreateIntermediate(wp);
      vector.push_back(tp);
    }
  }

public:
  /**
   * Constructor
   * @param wpv Vector to add to
   * @return Initialised object
   */
  MatWaypointVisitorVector(MatPoints::MatVector &wpv,
                           const AbstractTaskFactory &_factory)
    :vector(wpv), factory(_factory) {}

  /**
   * Visit method, adds result to vector
   * @param wp Waypoint that is visited
   */
  void Visit(const Waypoint& wp) {
    if (wp.IsTurnpoint() && vector.size() <= MAX_MAT_POINTS)
      AddUnique(wp);
  }
  /**
   * number of points in file
   */
  unsigned Size() {
    return vector.size();
  }

};

void
MatPoints::FillMatPoints(const Waypoints &wps,
                         const AbstractTaskFactory &factory,
                         const GeoPoint &center)
{
  points.reserve(MatWaypointVisitorVector::MAX_MAT_POINTS);
  MatWaypointVisitorVector wvv(points, factory);
  if (wps.size() <= MatWaypointVisitorVector::MAX_MAT_POINTS)
    wps.VisitNamePrefix(_T(""), wvv);
  else {
    wps.VisitWithinRange(center,
                         fixed(MatWaypointVisitorVector::OPTIONAL_RANGE_LIMIT * 500),
                         wvv);
    if (wvv.Size() <= MatWaypointVisitorVector::MAX_MAT_POINTS)
      wps.VisitWithinRange(center,
                           fixed(MatWaypointVisitorVector::OPTIONAL_RANGE_LIMIT * 750),
                           wvv);
    if (wvv.Size() <= MatWaypointVisitorVector::MAX_MAT_POINTS)
      wps.VisitWithinRange(center,
                           fixed(MatWaypointVisitorVector::OPTIONAL_RANGE_LIMIT * 1000),
                           wvv);
  }
}

void
MatPoints::ClearMatPoints()
{
  if (points.empty())
    return;

  for (auto v = points.begin(); v != points.end(); ) {
    delete *v;
    points.erase(v);
  }
  assert(points.empty());
}

void
MatPoints::CloneFrom(const MatPoints &other,
                     const TaskBehaviour &task_behaviour,
                     const OrderedTaskSettings &ordered_task_settings)
{
  points.clear();

  points.reserve(other.GetMatPoints().size());
  for (const auto *i : other.GetMatPoints())
    points.push_back(i->Clone(task_behaviour, ordered_task_settings));
}
