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

#include "StartPoint.hpp"
#include "Task/Ordered/Settings.hpp"
#include "Task/ObservationZones/Boundary.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Geo/Math.hpp"

#include <assert.h>

StartPoint::StartPoint(ObservationZonePoint *_oz,
                       WaypointPtr &&wp,
                       const TaskBehaviour &tb,
                       const StartConstraints &_constraints)
  :OrderedTaskPoint(TaskPointType::START, _oz, std::move(wp), false),
   safety_height(tb.safety_height_arrival),
   margins(tb.start_margins),
   constraints(_constraints)
{
}

void
StartPoint::SetTaskBehaviour(const TaskBehaviour &tb)
{
  safety_height = tb.safety_height_arrival;
  margins = tb.start_margins;
}

double
StartPoint::GetElevation() const
{
  return GetBaseElevation() + safety_height;
}

void
StartPoint::SetOrderedTaskSettings(const OrderedTaskSettings &settings)
{
  OrderedTaskPoint::SetOrderedTaskSettings(settings);
  constraints = settings.start_constraints;
}

void
StartPoint::SetNeighbours(OrderedTaskPoint *_prev, OrderedTaskPoint *_next)
{
  assert(_prev==NULL);
  // should not ever have an inbound leg
  OrderedTaskPoint::SetNeighbours(_prev, _next);
}


void
StartPoint::find_best_start(const AircraftState &state,
                            const OrderedTaskPoint &next,
                            const FlatProjection &projection)
{
  /* check which boundary point results in the smallest distance to
     fly */

  const OZBoundary boundary = GetBoundary();
  assert(!boundary.empty());

  const auto end = boundary.end();
  auto i = boundary.begin();
  assert(i != end);

  const GeoPoint &next_location = next.GetLocationRemaining();

  GeoPoint best_location = *i;
  auto best_distance = ::DoubleDistance(state.location, *i, next_location);

  for (++i; i != end; ++i) {
    auto distance = ::DoubleDistance(state.location, *i, next_location);
    if (distance < best_distance) {
      best_location = *i;
      best_distance = distance;
    }
  }

  SetSearchMin(SearchPoint(best_location, projection));
}

bool
StartPoint::IsInSector(const AircraftState &state) const
{
  return OrderedTaskPoint::IsInSector(state) &&
    // TODO: not using margins?
    constraints.CheckHeight(state, GetBaseElevation());
}

bool
StartPoint::CheckExitTransition(const AircraftState &ref_now,
                                const AircraftState &ref_last) const
{
  if (!constraints.open_time_span.HasBegun(RoughTime::FromSecondOfDayChecked(unsigned(ref_last.time))))
    /* the start gate is not yet open when we left the OZ */
    return false;

  if (constraints.open_time_span.HasEnded(RoughTime::FromSecondOfDayChecked(unsigned(ref_now.time))))
    /* the start gate was already closed when we left the OZ */
    return false;

  if (!constraints.CheckSpeed(ref_now.ground_speed, &margins) ||
      !constraints.CheckSpeed(ref_last.ground_speed, &margins))
    /* flying too fast */
    return false;

  // TODO: not using margins?
  const bool now_in_height =
    constraints.CheckHeight(ref_now, GetBaseElevation());
  const bool last_in_height =
    constraints.CheckHeight(ref_last, GetBaseElevation());

  if (now_in_height && last_in_height) {
    // both within height limit, so use normal location checks
    return OrderedTaskPoint::CheckExitTransition(ref_now, ref_last);
  }
  if (!TransitionConstraint(ref_now.location, ref_last.location)) {
    // don't allow vertical crossings for line OZ's
    return false;
  }

  // transition inside sector to above
  return !now_in_height && last_in_height
    && OrderedTaskPoint::IsInSector(ref_last)
    && CanStartThroughTop();
}
