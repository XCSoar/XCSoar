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

#include "TaskAdvanceSmart.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Points/StartPoint.hpp"
#include "Points/AATPoint.hpp"
#include "Points/IntermediatePoint.hpp"
#include "Navigation/Aircraft.hpp"
#include "Task/Factory/TaskFactoryConstraints.hpp"

TaskAdvanceSmart::TaskAdvanceSmart()
  :state(TaskAdvance::MANUAL),
   start_requires_arm(false)
{
}

void
TaskAdvanceSmart::SetFactoryConstraints(const TaskFactoryConstraints &constraints)
{
  start_requires_arm = constraints.start_requires_arm;
}

bool
TaskAdvanceSmart::CheckReadyToAdvance(const TaskPoint &tp,
                                      const AircraftState &aircraft,
                                      const bool x_enter, const bool x_exit)
{
  const bool state_ready = IsStateReady(tp, aircraft, x_enter, x_exit);

  if (armed)
    request_armed = false;

  if (tp.GetType() == TaskPoint::START) {
    const StartPoint *sp = (const StartPoint *)&tp;
    if (start_requires_arm) {
      if (armed) {
        state = TaskAdvance::START_ARMED;
      } else {
        state = TaskAdvance::START_DISARMED;
        if (sp->IsInSector(aircraft))
          request_armed = true;
      }
      return armed && state_ready;
    } else {
      state = TaskAdvance::AUTO;
      return state_ready;
    }
  } else if (tp.GetType() == TaskPoint::AAT) {
    if (armed) {
      state = TaskAdvance::TURN_ARMED;
    } else {
      state = TaskAdvance::TURN_DISARMED;
      if (state_ready)
        request_armed = true;
    }
    return armed && state_ready;
  } else if (tp.IsIntermediatePoint()) {
    state = TaskAdvance::AUTO;
    return state_ready;
  }

  return false;
}

TaskAdvance::State
TaskAdvanceSmart::GetState() const
{
  return state;
}

void
TaskAdvanceSmart::UpdateState()
{
  switch (state) {
  case TaskAdvance::START_ARMED:
    if (!armed)
      state = TaskAdvance::START_DISARMED;

    return;
  case TaskAdvance::START_DISARMED:
    if (armed)
      state = TaskAdvance::START_ARMED;

    return;
  case TaskAdvance::TURN_ARMED:
    if (!armed)
      state = TaskAdvance::TURN_DISARMED;

    return;
  case TaskAdvance::TURN_DISARMED:
    if (armed)
      state = TaskAdvance::TURN_ARMED;

    return;
  default:
    break;
  };
}
