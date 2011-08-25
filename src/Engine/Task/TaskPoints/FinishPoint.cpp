/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
                         const OrderedTaskBehaviour& to)
  :OrderedTaskPoint(FINISH, _oz, wp, to),
   safety_height_arrival(tb.safety_height_arrival),
   fai_finish_height(fixed_zero)
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
  fai_finish_height = fixed_zero;
}

bool 
FinishPoint::entry_precondition() const
{
  return get_previous() != NULL && get_previous()->HasEntered();
}

fixed
FinishPoint::GetElevation() const
{
  const fixed nominal_elevation = GetBaseElevation() + safety_height_arrival;

  if (m_ordered_task_behaviour.fai_finish) {
    return max(nominal_elevation, fai_finish_height);
  } else {
    return max(nominal_elevation,
               fixed(m_ordered_task_behaviour.finish_min_height) +
               (m_ordered_task_behaviour.finish_min_height_ref == hrAGL ?
                 GetBaseElevation() : fixed_zero));
  }
}


void 
FinishPoint::set_neighbours(OrderedTaskPoint* _prev, OrderedTaskPoint* _next)
{
  assert(_next == NULL);
  // should not ever have an outbound leg
  OrderedTaskPoint::set_neighbours(_prev, _next);
}

void 
FinishPoint::set_fai_finish_height(const fixed height)
{
  fai_finish_height = max(fixed_zero, height);
}

bool 
FinishPoint::IsInSector(const AircraftState &state) const
{
  if (!ObservationZoneClient::IsInSector(state)) 
    return false;

  return is_in_height_limit(state);
}

bool
FinishPoint::is_in_height_limit(const AircraftState &state) const
{
  if (!m_ordered_task_behaviour.check_finish_height(state, GetBaseElevation()))
    return false;

  if (m_ordered_task_behaviour.fai_finish)
    return state.altitude > fai_finish_height;

  return true;
}

bool 
FinishPoint::CheckEnterTransition(const AircraftState & ref_now, 
                                    const AircraftState & ref_last) const
{
  const bool now_in_height = is_in_height_limit(ref_now);
  const bool last_in_height = is_in_height_limit(ref_last);

  if (now_in_height && last_in_height)
    // both within height limit, so use normal location checks
    return ObservationZone::CheckEnterTransition(ref_now, ref_last);

  return false;
}
