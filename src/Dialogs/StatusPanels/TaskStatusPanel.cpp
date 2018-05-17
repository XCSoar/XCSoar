/*
Copyright_License {

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

#include "TaskStatusPanel.hpp"
#include "Form/DataField/Float.hpp"
#include "ActionInterface.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Language/Language.hpp"

enum Controls {
  TaskTime,
  ETETime,
  RemainingTime,
  TaskDistance,
  RemainingDistance,
  EstimatedSpeed,
  AverageSpeed,

  MC,
  RANGE,
  SPEED_REMAINING,
  EFFECTIVE_MC,
  SPEED_ACHIEVED,
  CRUISE_EFFICIENCY,
};

void
TaskStatusPanel::OnModified(DataField &df)
{
  if (IsDataField(MC, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    auto mc = Units::ToSysVSpeed(dff.GetAsFixed());
    ActionInterface::SetManualMacCready(mc);
    Refresh();
  }
}

void
TaskStatusPanel::Refresh()
{
  if (protected_task_manager == nullptr)
    return;

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;

  SetRowVisible(TaskTime, task_stats.has_targets);
  if (task_stats.has_targets)
    SetText(TaskTime,
            FormatSignedTimeHHMM((int)protected_task_manager->GetOrderedTaskSettings().aat_min_time));

  SetText(ETETime,
          FormatSignedTimeHHMM((int)task_stats.GetEstimatedTotalTime()));

  SetText(RemainingTime,
          FormatSignedTimeHHMM((int)task_stats.total.time_remaining_now));

  if (task_stats.total.planned.IsDefined())
    SetText(TaskDistance,
            FormatUserDistanceSmart(task_stats.total.planned.GetDistance()));
  else
    ClearText(TaskDistance);

  if (task_stats.total.remaining.IsDefined())
    SetText(RemainingDistance,
            FormatUserDistanceSmart(task_stats.total.remaining.GetDistance()));

  if (task_stats.total.planned.IsDefined())
    SetText(EstimatedSpeed,
            FormatUserTaskSpeed(task_stats.total.planned.GetSpeed()));
  else
    ClearText(EstimatedSpeed);

  if (task_stats.total.travelled.IsDefined())
    SetText(AverageSpeed,
            FormatUserTaskSpeed(task_stats.total.travelled.GetSpeed()));
  else
    ClearText(AverageSpeed);

  LoadValue(MC,
            CommonInterface::GetComputerSettings().polar.glide_polar_task.GetMC(),
            UnitGroup::VERTICAL_SPEED);

  if (task_stats.total.planned.IsDefined()) {
    auto rMax = task_stats.distance_max;
    auto rMin = task_stats.distance_min;

    if (rMin < rMax) {
      auto range = 2 * (task_stats.total.planned.GetDistance() - rMin) / (rMax - rMin) - 1;
      LoadValue(RANGE, range * 100);
    } else
      ClearValue(RANGE);
  } else
    ClearValue(RANGE);

  if (task_stats.total.remaining_effective.IsDefined())
    LoadValue(SPEED_REMAINING, task_stats.total.remaining_effective.GetSpeed(),
              UnitGroup::TASK_SPEED);
  else
    ClearValue(SPEED_REMAINING);

  LoadValue(EFFECTIVE_MC, task_stats.effective_mc, UnitGroup::VERTICAL_SPEED);

  if (task_stats.total.travelled.IsDefined())
    LoadValue(SPEED_ACHIEVED, task_stats.total.travelled.GetSpeed(),
              UnitGroup::TASK_SPEED);
  else
    ClearValue(SPEED_ACHIEVED);

  LoadValue(CRUISE_EFFICIENCY, task_stats.cruise_efficiency * 100);
}

void
TaskStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddReadOnly(_("Assigned task time"));
  AddReadOnly(_("Estimated task time"));
  AddReadOnly(_("Remaining time"));
  AddReadOnly(_("Task distance"));
  AddReadOnly(_("Remaining distance"));
  AddReadOnly(_("Speed estimated"));
  AddReadOnly(_("Speed average"));

  AddFloat(_("Set MacCready"),
           _("Adjusts MC value used in the calculator.  "
             "Use this to determine the effect on estimated task time due to changes in conditions.  "
             "This value will not affect the main computer's setting if the dialog is exited with the Cancel button."),
           _T("%.1f %s"), _T("%.1f"),
           0, Units::ToUserVSpeed(5),
           GetUserVerticalSpeedStep(), false, 0,
           this);
  DataFieldFloat &mc_df = (DataFieldFloat &)GetDataField(MC);
  mc_df.SetFormat(GetUserVerticalSpeedFormat(false, false));

  AddReadOnly(_("AAT range"),
              /* xgettext:no-c-format */
              _("For AAT tasks, this value tells you how far based on the targets of your task you will fly relative to the minimum and maximum possible tasks. -100% indicates the minimum AAT distance.  0% is the nominal AAT distance.  +100% is maximum AAT distance."),
              _T("%.0f %%"), 0);

  AddReadOnly(_("Speed remaining"), nullptr, _T("%.0f %s"),
              UnitGroup::TASK_SPEED, 0);

  AddReadOnly(_("Achieved MacCready"), nullptr, _T("%.1f %s"),
              UnitGroup::VERTICAL_SPEED, 0);
  DataFieldFloat &emc_df = (DataFieldFloat &)GetDataField(EFFECTIVE_MC);
  emc_df.SetFormat(GetUserVerticalSpeedFormat(false, false));

  AddReadOnly(_("Achieved speed"), nullptr, _T("%.0f %s"),
              UnitGroup::TASK_SPEED, 0);

  AddReadOnly(_("Cruise efficiency"),
              _("Efficiency of cruise.  100 indicates perfect MacCready performance, greater than 100 indicates better than MacCready performance is achieved through flying in streets.  Less than 100 is appropriate if you fly considerably off-track.  This value estimates your cruise efficiency according to the current flight history with the set MC value.  Calculation begins after task is started."),
              _T("%.0f %%"),
              0);
}
