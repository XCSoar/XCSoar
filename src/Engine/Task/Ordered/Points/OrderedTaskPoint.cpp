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

#include "OrderedTaskPoint.hpp"
#include "StartPoint.hpp"
#include "ASTPoint.hpp"
#include "AATPoint.hpp"
#include "FinishPoint.hpp"
#include "Task/ObservationZones/ObservationZonePoint.hpp"
#include "Task/ObservationZones/Boundary.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/Math.hpp"

#include <assert.h>

OrderedTaskPoint::OrderedTaskPoint(TaskPointType _type,
                                   ObservationZonePoint *_oz,
                                   WaypointPtr &&wp,
                                   const bool b_scored)
  :TaskLeg(*this),
   TaskWaypoint(_type, std::move(wp)),
   ScoredTaskPoint(GetLocation(), b_scored),
   ObservationZoneClient(_oz),
   tp_next(NULL), tp_previous(NULL),
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

void
OrderedTaskPoint::UpdateGeometry()
{
  SetLegs(tp_previous, tp_next);
}

void
OrderedTaskPoint::UpdateOZ(const FlatProjection &projection)
{
  UpdateGeometry();

  SampledTaskPoint::UpdateOZ(projection, GetBoundary());
}

bool
OrderedTaskPoint::ScanActive(const OrderedTaskPoint &atp)
{
  if (&atp == this)
    active_state = CURRENT_ACTIVE;
  else if (tp_previous &&
           (GetPrevious()->GetActiveState() == CURRENT_ACTIVE ||
            GetPrevious()->GetActiveState() == AFTER_ACTIVE))
    active_state = AFTER_ACTIVE;
  else
    active_state = BEFORE_ACTIVE;

  SetPast(IsPast());

  if (tp_next)
    // propagate to remainder of task
    return GetNext()->ScanActive(atp);

  return !IsPast();
}

const SearchPointVector &
OrderedTaskPoint::GetSearchPoints() const
{
  if (IsFuture())
    return GetBoundaryPoints();

  return SampledTaskPoint::GetSearchPoints();
}

bool
OrderedTaskPoint::IsInSector(const AircraftState &ref) const
{
  return ObservationZoneClient::IsInSector(ref.location);
}

bool
OrderedTaskPoint::UpdateSampleNear(const AircraftState &state,
                                   const FlatProjection &projection)
{
  if (!IsInSector(state))
    // return false (no update required)
    return false;

  return AddInsideSample(state, projection);
}

bool
OrderedTaskPoint::CheckEnterTransition(const AircraftState &ref_now,
                                       const AircraftState &ref_last) const
{
  return IsInSector(ref_now) && !IsInSector(ref_last) &&
    TransitionConstraint(ref_now.location, ref_last.location);
}

double
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

OrderedTaskPoint *
OrderedTaskPoint::Clone(const TaskBehaviour &task_behaviour,
                        const OrderedTaskSettings &ordered_task_settings,
                        WaypointPtr &&waypoint) const
{
  if (!waypoint)
    waypoint = GetWaypointPtr();

  switch (GetType()) {
  case TaskPointType::START:
    return new StartPoint(GetObservationZone().Clone(waypoint->location),
                          std::move(waypoint), task_behaviour,
                          ordered_task_settings.start_constraints);

  case TaskPointType::AST: {
    const ASTPoint &src = *(const ASTPoint *)this;
    ASTPoint *dest =
      new ASTPoint(GetObservationZone().Clone(waypoint->location),
                   std::move(waypoint), task_behaviour, IsBoundaryScored());
    dest->SetScoreExit(src.GetScoreExit());
    return dest;
  }

  case TaskPointType::AAT:
    return new AATPoint(GetObservationZone().Clone(waypoint->location),
                        std::move(waypoint), task_behaviour);

  case TaskPointType::FINISH:
    return new FinishPoint(GetObservationZone().Clone(waypoint->location),
                           std::move(waypoint), task_behaviour,
                           ordered_task_settings.finish_constraints,
                           IsBoundaryScored());

  case TaskPointType::UNORDERED:
    /* an OrderedTaskPoint must never be UNORDERED */
    gcc_unreachable();
    assert(false);
    break;
  }

  return NULL;
}

void
OrderedTaskPoint::ScanBounds(GeoBounds &bounds) const
{
  bounds.Extend(GetLocation());

  for (const auto &i : GetBoundary())
    bounds.Extend(i);
}

void
OrderedTaskPoint::UpdateBoundingBox(const FlatProjection &projection)
{
  flat_bb = FlatBoundingBox(projection.ProjectInteger(GetLocation()));

  for (const auto &i : GetBoundary())
    flat_bb.Expand(projection.ProjectInteger(i));

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
