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

#include "FinishPoint.hpp"
#include "Task/TaskBehaviour.hpp"

#include <stdlib.h>
#include <assert.h>

FinishPoint::FinishPoint(ObservationZonePoint* _oz, const Waypoint & wp,
                         const TaskBehaviour& tb,
                         const FinishConstraints &_constraints,
                         bool boundary_scored)
  :OrderedTaskPoint(TaskPointType::FINISH, _oz, wp, boundary_scored),
   safety_height_arrival(tb.safety_height_arrival),
   constraints(_constraints),
   fai_finish_height(fixed(0))
{
}

void
FinishPoint::SetTaskBehaviour(const TaskBehaviour &tb)
{
  safety_height_arrival = tb.safety_height_arrival;
}

void
FinishPoint::Reset()
{
  OrderedTaskPoint::Reset();
  fai_finish_height = fixed(0);
}

bool
FinishPoint::EntryPrecondition() const
{
  return GetPrevious() != NULL && GetPrevious()->HasEntered();
}

fixed
FinishPoint::GetElevation() const
{
  const fixed nominal_elevation = GetBaseElevation() + safety_height_arrival;

  if (constraints.fai_finish) {
    return std::max(nominal_elevation, fai_finish_height);
  } else {
    return std::max(nominal_elevation,
                    fixed(constraints.min_height) +
                    (constraints.min_height_ref == AltitudeReference::AGL
                     ? GetBaseElevation() : fixed(0)));
  }
}

void
FinishPoint::SetOrderedTaskBehaviour(const OrderedTaskBehaviour &otb)
{
  OrderedTaskPoint::SetOrderedTaskBehaviour(otb);
  constraints = otb.finish_constraints;
}

void
FinishPoint::SetNeighbours(OrderedTaskPoint *_prev, OrderedTaskPoint *_next)
{
  assert(_next == NULL);
  // should not ever have an outbound leg
  OrderedTaskPoint::SetNeighbours(_prev, _next);
}

void
FinishPoint::set_fai_finish_height(const fixed height)
{
  fai_finish_height = std::max(fixed(0), height);
}

bool
FinishPoint::IsInSector(const AircraftState &state) const
{
  if (!OrderedTaskPoint::IsInSector(state))
    return false;

  return is_in_height_limit(state);
}

bool
FinishPoint::is_in_height_limit(const AircraftState &state) const
{
  if (!constraints.CheckHeight(state, GetBaseElevation()))
    return false;

  if (constraints.fai_finish)
    return state.altitude > fai_finish_height;

  return true;
}

bool
FinishPoint::CheckEnterTransition(const AircraftState &ref_now,
                                  const AircraftState &ref_last) const
{
  const bool now_in_height = is_in_height_limit(ref_now);
  const bool last_in_height = is_in_height_limit(ref_last);

  if (now_in_height && last_in_height)
    // both within height limit, so use normal location checks
    return OrderedTaskPoint::CheckEnterTransition(ref_now, ref_last);

  return false;
}
