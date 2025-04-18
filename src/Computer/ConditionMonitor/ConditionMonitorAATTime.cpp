// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitorAATTime.hpp"
#include "NMEA/Derived.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"

bool
ConditionMonitorAATTime::CheckCondition([[maybe_unused]] const NMEAInfo &basic,
                                        const DerivedInfo &calculated,
                                        [[maybe_unused]] const ComputerSettings &settings) noexcept
{
  if (!calculated.flight.flying ||
      calculated.common_stats.task_type != TaskType::ORDERED ||
      !calculated.ordered_task_stats.task_valid ||
      !calculated.ordered_task_stats.has_targets ||
      !calculated.ordered_task_stats.start.HasStarted() ||
      !calculated.common_stats.active_has_next ||
      calculated.ordered_task_stats.task_finished)
    return false;

  return calculated.ordered_task_stats.total.time_remaining_now <
    calculated.common_stats.aat_time_remaining;
}

void
ConditionMonitorAATTime::Notify() noexcept
{
  Message::AddMessage(_("Expect early task arrival"));
}
