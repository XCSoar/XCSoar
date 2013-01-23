/* Copyright_License {

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

#include "OrderedTaskPoint.hpp"
#include "StartPoint.hpp"
#include "ASTPoint.hpp"
#include "AATPoint.hpp"
#include "FinishPoint.hpp"
#include "Task/ObservationZones/ObservationZonePoint.hpp"
#include "Task/ObservationZones/Boundary.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Geo/Math.hpp"

#include <assert.h>
#include <math.h>

OrderedTaskPoint::OrderedTaskPoint(TaskPointType _type,
                                   ObservationZonePoint *_oz,
                                   const Waypoint &wp,
                                   const bool b_scored)
  :TaskLeg(*this), ScoredTaskPoint(_type, wp, b_scored),
   ObservationZoneClient(_oz),
   active_state(NOTFOUND_ACTIVE), tp_next(NULL), tp_previous(NULL),
   flat_bb(FlatGeoPoint(0,0),0) // empty, not initialised!
{
}

void 
OrderedTaskPoint::SetNeighbours(OrderedTaskPoint *_previous,
                                OrderedTaskPoint *_next)
{
  tp_previous = _previous;
  tp_next = _next;

  UpdateGeometry();
}

/** 
 * Update observation zone geometry (or other internal data) when
 * previous/next turnpoint changes.
 */
void
OrderedTaskPoint::UpdateGeometry()
{
  SetLegs(tp_previous, this, tp_next);
}

void
OrderedTaskPoint::UpdateOZ(const TaskProjection &projection)
{
  UpdateGeometry();

  SampledTaskPoint::UpdateOZ(projection);
}

bool
OrderedTaskPoint::ScanActive(const OrderedTaskPoint &atp)
{
  // reset
  active_state = NOTFOUND_ACTIVE;

  if (&atp == this)
    active_state = CURRENT_ACTIVE;
  else if (tp_previous &&
           (GetPrevious()->GetActiveState() == CURRENT_ACTIVE ||
            GetPrevious()->GetActiveState() == AFTER_ACTIVE))
    active_state = AFTER_ACTIVE;
  else
    active_state = BEFORE_ACTIVE;

  if (tp_next)
    // propagate to remainder of task
    return GetNext()->ScanActive(atp);

  return active_state != BEFORE_ACTIVE && active_state != NOTFOUND_ACTIVE;
}

bool
OrderedTaskPoint::SearchBoundaryPoints() const
{
  return active_state == AFTER_ACTIVE;
}

bool
OrderedTaskPoint::SearchNominalIfUnsampled() const
{
  return active_state == BEFORE_ACTIVE;
}

bool
OrderedTaskPoint::IsInSector(const AircraftState &ref) const
{
  return ObservationZoneClient::IsInSector(ref.location);
}

OZBoundary
OrderedTaskPoint::GetBoundary() const
{
  return ObservationZoneClient::GetBoundary();
}

bool
OrderedTaskPoint::CheckEnterTransition(const AircraftState &ref_now,
                                       const AircraftState &ref_last) const
{
  return IsInSector(ref_now) && !IsInSector(ref_last) &&
    TransitionConstraint(ref_now.location, ref_last.location);
}

fixed 
OrderedTaskPoint::DoubleLegDistance(const GeoPoint &ref) const
{
  assert(tp_previous);
  assert(tp_next);

  return ::DoubleDistance(GetPrevious()->GetLocationRemaining(),
                          ref, GetNext()->GetLocationRemaining());
}

bool 
OrderedTaskPoint::Equals(const OrderedTaskPoint &other) const
{
  return GetWaypoint() == other.GetWaypoint() &&
    GetType() == other.GetType() &&
    GetObservationZone().Equals(other.GetObservationZone()) &&
    other.GetObservationZone().Equals(GetObservationZone());
}

OrderedTaskPoint* 
OrderedTaskPoint::Clone(const TaskBehaviour &task_behaviour,
                        const OrderedTaskBehaviour &ordered_task_behaviour,
                        const Waypoint* waypoint) const
{
  if (waypoint == NULL)
    waypoint = &GetWaypoint();

  switch (GetType()) {
  case TaskPointType::START:
    return new StartPoint(GetObservationZone().Clone(waypoint->location),
                          *waypoint, task_behaviour,
                          ordered_task_behaviour.start_constraints);

  case TaskPointType::AST:
    return new ASTPoint(GetObservationZone().Clone(waypoint->location),
                        *waypoint, task_behaviour, IsBoundaryScored());

  case TaskPointType::AAT:
    return new AATPoint(GetObservationZone().Clone(waypoint->location),
                        *waypoint, task_behaviour);

  case TaskPointType::FINISH:
    return new FinishPoint(GetObservationZone().Clone(waypoint->location),
                           *waypoint, task_behaviour,
                           ordered_task_behaviour.finish_constraints,
                           IsBoundaryScored());

  case TaskPointType::UNORDERED:
    /* an OrderedTaskPoint must never be UNORDERED */
    assert(false);
    break;
  }

  return NULL;
}

void
OrderedTaskPoint::ScanProjection(TaskProjection &task_projection) const
{
  task_projection.Scan(GetLocation());

  for (const auto &i : GetBoundary())
    task_projection.Scan(i);
}

void
OrderedTaskPoint::UpdateBoundingBox(const TaskProjection &task_projection)
{
  flat_bb = FlatBoundingBox(task_projection.ProjectInteger(GetLocation()));

  for (const auto &i : GetBoundary())
    flat_bb.Expand(task_projection.ProjectInteger(i));

  flat_bb.ExpandByOne(); // add 1 to fix rounding
}

bool
OrderedTaskPoint::BoundingBoxOverlaps(const FlatBoundingBox &that) const
{
  return flat_bb.Overlaps(that);
}

GeoVector
OrderedTaskPoint::GetNextLegVector() const
{
  if (tp_next)
    return tp_next->GetVectorPlanned();

  return GeoVector::Invalid();
}
