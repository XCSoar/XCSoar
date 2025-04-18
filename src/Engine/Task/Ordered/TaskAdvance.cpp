// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskAdvance.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Points/StartPoint.hpp"
#include "Points/AATPoint.hpp"
#include "Points/ASTPoint.hpp"
#include "util/Compiler.h"

void
TaskAdvance::Reset() noexcept
{
  armed = false;
  request_armed = false;
}

bool
TaskAdvance::IsStateReady(const TaskPoint &tp,
                          const AircraftState &state,
                          [[maybe_unused]] const bool x_enter,
                          const bool x_exit) const noexcept
{
  switch (tp.GetType()) {
  case TaskPointType::UNORDERED:
    gcc_unreachable();

  case TaskPointType::START: {
    const auto &sp = (const StartPoint &)tp;
    return sp.GetScoreExit()
      ? x_exit
      : sp.HasEntered();
  }

  case TaskPointType::AAT: {
    const AATPoint &ap = (const AATPoint &)tp;
    return IsAATStateReady(ap.HasEntered(), ap.IsCloseToTarget(state));
  }

  case TaskPointType::AST: {
    const ASTPoint &ip = (const ASTPoint &)tp;
    return ip.GetScoreExit()
      ? x_exit
      : ip.HasEntered();
  }

  case TaskPointType::FINISH:
    return false;
  }

  gcc_unreachable();
}

bool
TaskAdvance::IsAATStateReady(const bool has_entered,
                             [[maybe_unused]] const bool close_to_target) const noexcept
{
  return has_entered;
}

void
TaskAdvance::SetArmed(const bool do_armed) noexcept
{
  armed = do_armed;
  request_armed = false;
  UpdateState();
}

bool
TaskAdvance::ToggleArmed() noexcept
{
  armed = !armed;
  if (armed)
    request_armed = false;

  UpdateState();
  return armed;
}
