// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskConstraintsMonitor.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Interface.hpp"
#include "Message.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "util/StaticString.hxx"
#include "Components.hpp"
#include "BackendComponents.hpp"

[[gnu::pure]]
static bool
InStartSector()
{
  const auto &calculated = CommonInterface::Calculated();

  return calculated.flight.flying &&
    calculated.ordered_task_stats.task_valid &&
    calculated.ordered_task_stats.active_index == 0 &&
    calculated.ordered_task_stats.inside_oz;
}

void
TaskConstraintsMonitor::Check()
{
  const auto &basic = CommonInterface::Basic();

  if (InStartSector()) {
    const auto settings = backend_components->protected_task_manager->GetOrderedTaskSettings();

    if (basic.ground_speed_available &&
        !settings.start_constraints.CheckSpeed(basic.ground_speed) &&
        max_start_speed_clock.CheckUpdate(std::chrono::seconds(30))) {
      StaticString<256> msg;
      msg.Format("%s (%s > %s)", _("Maximum start speed exceeded"),
                 FormatUserSpeed(basic.ground_speed).c_str(),
                 FormatUserSpeed(settings.start_constraints.max_speed).c_str());
      Message::AddMessage(msg);
    }
  } else {
    max_start_speed_clock.Reset();
  }
}
