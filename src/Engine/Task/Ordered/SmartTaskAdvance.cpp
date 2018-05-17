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

#include "SmartTaskAdvance.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Points/StartPoint.hpp"

SmartTaskAdvance::SmartTaskAdvance()
  :state(TaskAdvance::MANUAL)
{
}

bool
SmartTaskAdvance::CheckReadyToAdvance(const TaskPoint &tp,
                                      const AircraftState &aircraft,
                                      const bool x_enter, const bool x_exit)
{
  const bool state_ready = IsStateReady(tp, aircraft, x_enter, x_exit);

  if (armed)
    request_armed = false;

  switch (tp.GetType()) {
  case TaskPointType::UNORDERED:
    gcc_unreachable();

  case TaskPointType::START: {
    const StartPoint &sp = (const StartPoint &)tp;
    if (sp.DoesRequireArm()) {
      if (armed) {
        state = TaskAdvance::START_ARMED;
      } else {
        state = TaskAdvance::START_DISARMED;
        if (sp.IsInSector(aircraft))
          request_armed = true;
      }
      return armed && state_ready;
    } else {
      state = TaskAdvance::AUTO;
      return state_ready;
    }
  }

  case TaskPointType::AAT:
    if (armed) {
      state = TaskAdvance::TURN_ARMED;
    } else {
      state = TaskAdvance::TURN_DISARMED;
      if (state_ready)
        request_armed = true;
    }
    return armed && state_ready;

  case TaskPointType::AST: {
    state = TaskAdvance::AUTO;
    return state_ready;
  }

  case TaskPointType::FINISH:
    return false;
  }

  gcc_unreachable();
}

TaskAdvance::State
SmartTaskAdvance::GetState() const
{
  return state;
}

void
SmartTaskAdvance::UpdateState()
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
