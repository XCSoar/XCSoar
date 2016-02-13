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

#include "TaskRulesConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  StartMaxSpeed,
  StartMaxSpeedMargin,
  spacer_1,
  StartMaxHeight,
  StartMaxHeightMargin,
  StartHeightRef,
  spacer_2,
  FinishMinHeight,
  FinishHeightRef,
};

class TaskRulesConfigPanel final : public RowFormWidget {
public:
  TaskRulesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
};

void
TaskRulesConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  RowFormWidget::Prepare(parent, rc);

  AddFloat(_("Start max. speed"), _("Maximum speed allowed in start observation zone.  Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"), 0, 300, 5, false, UnitGroup::HORIZONTAL_SPEED,
           task_behaviour.ordered_defaults.start_constraints.max_speed);
  SetExpertRow(StartMaxSpeed);

  AddFloat(_("Start max. speed margin"),
           _("Maximum speed above maximum start speed to tolerate.  Set to 0 for no tolerance."),
           _T("%.0f %s"), _T("%.0f"), 0, 300, 5, false, UnitGroup::HORIZONTAL_SPEED,
           task_behaviour.start_margins.max_speed_margin);
  SetExpertRow(StartMaxSpeedMargin);

  AddSpacer();
  SetExpertRow(spacer_1);

  AddFloat(_("Start max. height"),
           _("Maximum height based on start height reference (AGL or MSL) while starting the task.  "
               "Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"), 0, 10000, 50, false, UnitGroup::ALTITUDE,
           task_behaviour.ordered_defaults.start_constraints.max_height);
  SetExpertRow(StartMaxHeight);

  AddFloat(_("Start max. height margin"),
           _("Maximum height above maximum start height to tolerate.  Set to 0 for no tolerance."),
           _T("%.0f %s"), _T("%.0f"), 0, 10000, 50, false, UnitGroup::ALTITUDE,
           task_behaviour.start_margins.max_height_margin);
  SetExpertRow(StartMaxHeightMargin);

  static constexpr StaticEnumChoice altitude_reference_list[] = {
    { (unsigned)AltitudeReference::AGL, N_("AGL"),
      N_("Reference is the height above the task point."), },
    { (unsigned)AltitudeReference::MSL, N_("MSL"),
      N_("Reference is altitude above mean sea level."), },
    { 0 }
  };

  AddEnum(_("Start height ref."),
          _("Reference used for start max height rule."),
          altitude_reference_list,
          (unsigned)task_behaviour.ordered_defaults.start_constraints.max_height_ref);
  SetExpertRow(StartHeightRef);

  AddSpacer();
  SetExpertRow(spacer_2);

  AddFloat(_("Finish min. height"),
           _("Minimum height based on finish height reference (AGL or MSL) while finishing the task.  "
               "Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"), 0, 10000, 50, false, UnitGroup::ALTITUDE,
           task_behaviour.ordered_defaults.finish_constraints.min_height);
  SetExpertRow(FinishMinHeight);

  AddEnum(_("Finish height ref."),
          _("Reference used for finish min height rule."),
          altitude_reference_list,
          (unsigned)task_behaviour.ordered_defaults.finish_constraints.min_height_ref);
  SetExpertRow(FinishHeightRef);
}


bool
TaskRulesConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;
  OrderedTaskSettings &otb = task_behaviour.ordered_defaults;

  changed |= SaveValue(StartMaxSpeed, UnitGroup::HORIZONTAL_SPEED,
                       ProfileKeys::StartMaxSpeed,
                       otb.start_constraints.max_speed);

  changed |= SaveValue(StartMaxSpeedMargin, UnitGroup::HORIZONTAL_SPEED, ProfileKeys::StartMaxSpeedMargin,
                       task_behaviour.start_margins.max_speed_margin);

  changed |= SaveValue(StartMaxHeight, UnitGroup::ALTITUDE,
                       ProfileKeys::StartMaxHeight,
                       otb.start_constraints.max_height);

  changed |= SaveValue(StartMaxHeightMargin, UnitGroup::ALTITUDE, ProfileKeys::StartMaxHeightMargin,
                       task_behaviour.start_margins.max_height_margin);

  changed |= SaveValueEnum(StartHeightRef, ProfileKeys::StartHeightRef,
                           otb.start_constraints.max_height_ref);

  changed |= SaveValue(FinishMinHeight, UnitGroup::ALTITUDE,
                       ProfileKeys::FinishMinHeight,
                       otb.finish_constraints.min_height);

  changed |= SaveValueEnum(FinishHeightRef, ProfileKeys::FinishHeightRef,
                           otb.finish_constraints.min_height_ref);

  _changed |= changed;

  return true;
}

Widget *
CreateTaskRulesConfigPanel()
{
  return new TaskRulesConfigPanel();
}
