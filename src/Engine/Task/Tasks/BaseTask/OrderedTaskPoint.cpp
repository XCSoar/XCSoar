/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "OrderedTaskPoint.hpp"
#include "Task/ObservationZones/ObservationZonePoint.hpp"
#include "Task/ObservationZones/Boundary.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Math/Earth.hpp"
#include <assert.h>
#include <math.h>

OrderedTaskPoint::OrderedTaskPoint(Type _type, ObservationZonePoint* _oz,
                                   const Waypoint &wp,
                                   const OrderedTaskBehaviour &to,
                                   const bool b_scored)
  :TaskLeg(*this), ScoredTaskPoint(_type, wp, b_scored),
   ObservationZoneClient(_oz), m_ordered_task_behaviour(to),
   m_active_state(NOTFOUND_ACTIVE), tp_next(NULL), tp_previous(NULL),
   flat_bb(FlatGeoPoint(0,0),0) // empty, not initialised!
{
}

void 
OrderedTaskPoint::set_neighbours(OrderedTaskPoint* _prev,
                                 OrderedTaskPoint* _next) 
{
  tp_previous = _prev;
  tp_next = _next;

  update_geometry();
}

/** 
 * Update observation zone geometry (or other internal data) when
 * previous/next turnpoint changes.
 */
void
OrderedTaskPoint::update_geometry()
{
  SetLegs(tp_previous, this, tp_next);
}

void
OrderedTaskPoint::UpdateOZ(const TaskProjection &projection)
{
  update_geometry();

  SampledTaskPoint::UpdateOZ(projection);
}

bool
OrderedTaskPoint::scan_active(OrderedTaskPoint* atp)
{
  // reset
  m_active_state = NOTFOUND_ACTIVE;

  if (atp == this)
    m_active_state = CURRENT_ACTIVE;
  else if (tp_previous &&
           (get_previous()->getActiveState() == CURRENT_ACTIVE ||
               get_previous()->getActiveState() == AFTER_ACTIVE))
    m_active_state = AFTER_ACTIVE;
  else
    m_active_state = BEFORE_ACTIVE;

  if (tp_next)
    // propagate to remainder of task
    return get_next()->scan_active(atp);

  return m_active_state != BEFORE_ACTIVE &&
         m_active_state != NOTFOUND_ACTIVE;
}

bool
OrderedTaskPoint::SearchBoundaryPoints() const
{
  return m_active_state == AFTER_ACTIVE;
}

bool
OrderedTaskPoint::SearchNominalIfUnsampled() const
{
  return m_active_state == BEFORE_ACTIVE;
}

fixed 
OrderedTaskPoint::double_leg_distance(const GeoPoint &ref) const
{
  assert(tp_previous);
  assert(tp_next);

  return ::DoubleDistance(get_previous()->GetLocationRemaining(), 
                          ref, get_next()->GetLocationRemaining());
}

bool 
OrderedTaskPoint::equals(const OrderedTaskPoint* other) const
{
  return GetWaypoint() == other->GetWaypoint() &&
         GetType() == other->GetType() &&
    GetOZPoint()->Equals(*other->GetOZPoint()) &&
    other->GetOZPoint()->Equals(*GetOZPoint());
}

OrderedTaskPoint* 
OrderedTaskPoint::clone(const TaskBehaviour &task_behaviour,
                        const OrderedTaskBehaviour &ordered_task_behaviour,
                        const Waypoint* waypoint) const
{
  if (waypoint == NULL)
    waypoint = &GetWaypoint();

  switch (GetType()) {
  case START:
    return new StartPoint(GetOZPoint()->Clone(waypoint->location),
                          *waypoint, task_behaviour, ordered_task_behaviour);

  case AST:
    return new ASTPoint(GetOZPoint()->Clone(waypoint->location),
                        *waypoint, task_behaviour, ordered_task_behaviour);

  case AAT:
    return new AATPoint(GetOZPoint()->Clone(waypoint->location),
                        *waypoint, task_behaviour, ordered_task_behaviour);

  case FINISH:
    return new FinishPoint(GetOZPoint()->Clone(waypoint->location),
                           *waypoint, task_behaviour, ordered_task_behaviour,
                           boundary_scored);

  case UNORDERED:
  case ROUTE:
    /* an OrderedTaskPoint must never be UNORDERED or ROUTE */
    assert(false);
    break;
  }

  return NULL;
}

void
OrderedTaskPoint::scan_projection(TaskProjection &task_projection) const
{
  task_projection.scan_location(GetLocation());
  #define fixed_steps fixed(0.05)

  const OZBoundary boundary = GetBoundary();
  for (auto i = boundary.begin(), end = boundary.end(); i != end; ++i)
    task_projection.scan_location(*i);
}

void
OrderedTaskPoint::update_boundingbox(const TaskProjection &task_projection)
{
  flat_bb = FlatBoundingBox(task_projection.project(GetLocation()));

  const OZBoundary boundary = GetBoundary();
  for (auto i = boundary.begin(), end = boundary.end(); i != end; ++i)
    flat_bb.Expand(task_projection.project(*i));

  flat_bb.ExpandByOne(); // add 1 to fix rounding
}

bool
OrderedTaskPoint::boundingbox_overlaps(const FlatBoundingBox &that) const
{
  return flat_bb.Overlaps(that);
}
