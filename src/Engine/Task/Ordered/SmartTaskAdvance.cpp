// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SmartTaskAdvance.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Points/StartPoint.hpp"
#include "util/Compiler.h"

bool
SmartTaskAdvance::CheckReadyToAdvance(const TaskPoint &tp,
                                      const AircraftState &aircraft,
                                      const bool x_enter,
                                      const bool x_exit) noexcept
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
SmartTaskAdvance::GetState() const noexcept
{
  return state;
}

void
SmartTaskAdvance::UpdateState() noexcept
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
