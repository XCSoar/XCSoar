// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RulesStatusPanel.hpp"
#include "util/Macros.hpp"
#include "util/TruncateString.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Language/Language.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

#include <chrono>

enum Controls {
  ValidStart,
  StartTime,
  StartHeight,
  StartPoint,
  StartSpeed,
  PevOffsetAtStart,
  FinishAlt,
  ValidFinish,
};

void
RulesStatusPanel::Refresh() noexcept
{
  char Temp[80];

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const StartStats &start_stats = task_stats.start;
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  /// @todo proper task validity check
  SetText(ValidStart, start_stats.HasStarted()
          ? _("Yes") : "No");

  SetText(ValidFinish, task_stats.task_finished
          ? _("Yes") : "No");

  if (start_stats.HasStarted()) {
    SetText(StartTime,
            FormatLocalTimeHHMM(start_stats.time, settings.utc_offset));

    SetText(StartSpeed,
            FormatUserTaskSpeed(start_stats.ground_speed));

    SetText(StartHeight, FormatUserAltitude(start_stats.altitude));

    if (start_stats.pev_offset_available) {
      const auto buf = FormatTimespanSmart(
          std::chrono::seconds{start_stats.pev_offset_seconds}, 3);
      SetText(PevOffsetAtStart, buf.c_str());
    } else
      SetText(PevOffsetAtStart, _("—"));
  } else {
    ClearValue(StartTime);
    ClearValue(StartSpeed);
    ClearValue(StartHeight);
    ClearValue(PevOffsetAtStart);
  }

  Temp[0] = '\0';
  double finish_height(0);

  if (backend_components->protected_task_manager) {
    ProtectedTaskManager::Lease task_manager{*backend_components->protected_task_manager};
    const OrderedTask &task = task_manager->GetOrderedTask();
    const unsigned task_size = task.TaskSize();

    if (task_size > 0) {
      CopyTruncateString(Temp, ARRAY_SIZE(Temp),
                         task.GetTaskPoint(0).GetWaypoint().name.c_str());
      finish_height = task.GetTaskPoint(task_size - 1).GetElevation();
    }
  }

  SetText(StartPoint, Temp);

  SetText(FinishAlt, FormatUserAltitude(finish_height));
}

void
RulesStatusPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddReadOnly(_("Valid start"));
  AddReadOnly(_("Start time"));
  AddReadOnly(_("Start alt."));
  AddReadOnly(_("Start point"));
  AddReadOnly(_("Start speed"));
  AddReadOnly(_("PEV offset at start"));
  AddReadOnly(_("Finish min. alt."));
  AddReadOnly(_("Valid finish"));
}
