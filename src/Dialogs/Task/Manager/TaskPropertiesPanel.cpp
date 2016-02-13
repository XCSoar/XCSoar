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

#include "TaskPropertiesPanel.hpp"
#include "Internal.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Task/TypeStrings.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"

enum Controls {
  TASK_TYPE,
  MIN_TIME,
  START_REQUIRES_ARM,
  START_OPEN_TIME,
  START_CLOSE_TIME,
  START_MAX_SPEED,
  START_MAX_HEIGHT,
  START_HEIGHT_REF,
  FINISH_MIN_HEIGHT,
  FINISH_HEIGHT_REF,
  FAI_FINISH_HEIGHT,
};

TaskPropertiesPanel::TaskPropertiesPanel(TaskManagerDialog &_dialog,
                                         OrderedTask **_active_task,
                                         bool *_task_modified)
  :RowFormWidget(_dialog.GetLook()), dialog(_dialog),
   ordered_task_pointer(_active_task), ordered_task(*ordered_task_pointer),
   task_changed(_task_modified) {}

void
TaskPropertiesPanel::RefreshView()
{
  const TaskFactoryType ftype = ordered_task->GetFactoryType();
  const OrderedTaskSettings &p = ordered_task->GetOrderedTaskSettings();

  const bool aat_types = ftype == TaskFactoryType::AAT ||
    ftype == TaskFactoryType::MAT;
  bool fai_start_finish = p.finish_constraints.fai_finish;

  SetRowVisible(MIN_TIME, aat_types);
  LoadValueTime(MIN_TIME, (int)p.aat_min_time);

  LoadValue(START_REQUIRES_ARM, p.start_constraints.require_arm);

  LoadValue(START_OPEN_TIME, p.start_constraints.open_time_span.GetStart());
  LoadValue(START_CLOSE_TIME, p.start_constraints.open_time_span.GetEnd());

  SetRowVisible(START_MAX_SPEED, !fai_start_finish);
  LoadValue(START_MAX_SPEED, p.start_constraints.max_speed,
            UnitGroup::HORIZONTAL_SPEED);

  SetRowVisible(START_MAX_HEIGHT, !fai_start_finish);
  LoadValue(START_MAX_HEIGHT, double(p.start_constraints.max_height),
            UnitGroup::ALTITUDE);

  SetRowVisible(START_HEIGHT_REF, !fai_start_finish);
  LoadValueEnum(START_HEIGHT_REF, p.start_constraints.max_height_ref);

  SetRowVisible(FINISH_MIN_HEIGHT, !fai_start_finish);
  LoadValue(FINISH_MIN_HEIGHT, double(p.finish_constraints.min_height),
            UnitGroup::ALTITUDE);

  SetRowVisible(FINISH_HEIGHT_REF, !fai_start_finish);
  LoadValueEnum(FINISH_HEIGHT_REF, p.finish_constraints.min_height_ref);

  SetRowVisible(FAI_FINISH_HEIGHT, IsFai(ftype));
  LoadValue(FAI_FINISH_HEIGHT, fai_start_finish);

  LoadValueEnum(TASK_TYPE, ftype);

  dialog.InvalidateTaskView();

  // aat_min_time
  // finish_min_height
}

void
TaskPropertiesPanel::ReadValues()
{
  OrderedTaskSettings p = ordered_task->GetOrderedTaskSettings();
  bool changed = false;

  TaskFactoryType newtype = ordered_task->GetFactoryType();
  changed |= SaveValueEnum(TASK_TYPE, newtype);

  int min_time = GetValueInteger(MIN_TIME);
  if (min_time != (int)p.aat_min_time) {
    p.aat_min_time = min_time;
    changed = true;
  }

  if (SaveValue(START_REQUIRES_ARM, p.start_constraints.require_arm))
    changed = true;

  RoughTime new_open = p.start_constraints.open_time_span.GetStart();
  RoughTime new_close = p.start_constraints.open_time_span.GetEnd();
  const bool start_open_modified = SaveValue(START_OPEN_TIME, new_open);
  const bool start_close_modified = SaveValue(START_CLOSE_TIME, new_close);
  if (start_open_modified || start_close_modified) {
    p.start_constraints.open_time_span = RoughTimeSpan(new_open, new_close);
    changed = true;
  }

  auto max_speed = Units::ToSysSpeed(GetValueFloat(START_MAX_SPEED));
  if (max_speed != p.start_constraints.max_speed) {
    p.start_constraints.max_speed = max_speed;
    changed = true;
  }

  unsigned max_height =
    iround(Units::ToSysAltitude(GetValueFloat(START_MAX_HEIGHT)));
  if (max_height != p.start_constraints.max_height) {
    p.start_constraints.max_height = max_height;
    changed = true;
  }

  changed |= SaveValueEnum(START_HEIGHT_REF,
                           p.start_constraints.max_height_ref);

  unsigned min_height =
    iround(Units::ToSysAltitude(GetValueFloat(FINISH_MIN_HEIGHT)));
  if (min_height != p.finish_constraints.min_height) {
    p.finish_constraints.min_height = min_height;
    changed = true;
  }

  changed |= SaveValueEnum(FINISH_HEIGHT_REF,
                           p.finish_constraints.min_height_ref);

  if (changed)
    ordered_task->SetOrderedTaskSettings(p);

  *task_changed |= changed;
}

void
TaskPropertiesPanel::OnFAIFinishHeightChange(DataFieldBoolean &df)
{
  OrderedTaskSettings p = ordered_task->GetOrderedTaskSettings();
  bool newvalue = df.GetAsBoolean();
  if (newvalue != p.finish_constraints.fai_finish) {
    p.finish_constraints.fai_finish = p.start_constraints.fai_finish
      = newvalue;
    ordered_task->SetOrderedTaskSettings(p);

    *task_changed = true;
    RefreshView();
  }
}

void
TaskPropertiesPanel::OnTaskTypeChange(DataFieldEnum &df)
{
  const DataFieldEnum &dfe = (const DataFieldEnum &)df;
  const TaskFactoryType newtype = (TaskFactoryType)dfe.GetValue();
  if (newtype != ordered_task->GetFactoryType()) {
    ReadValues();
    ordered_task->SetFactory(newtype);
    *task_changed =true;
    RefreshView();
  }
}

void
TaskPropertiesPanel::OnModified(DataField &df)
{
  if (IsDataField(FAI_FINISH_HEIGHT, df))
    OnFAIFinishHeightChange((DataFieldBoolean &)df);
  else if (IsDataField(TASK_TYPE, df))
    OnTaskTypeChange((DataFieldEnum &)df);
}

void
TaskPropertiesPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  DataFieldEnum *dfe = new DataFieldEnum(this);
  dfe->EnableItemHelp(true);
  const std::vector<TaskFactoryType> factory_types =
    ordered_task->GetFactoryTypes();
  for (unsigned i = 0; i < factory_types.size(); i++) {
    dfe->addEnumText(OrderedTaskFactoryName(factory_types[i]),
                     (unsigned)factory_types[i],
                     OrderedTaskFactoryDescription(factory_types[i]));
    if (factory_types[i] == ordered_task->GetFactoryType())
      dfe->Set((unsigned)factory_types[i]);
  }
  Add(_("Task type"), _("Sets the behaviour for the current task."), dfe);

  AddTime(_("AAT min. time"), _("Minimum AAT task time in minutes."),
          0, 36000, 60, 180);

  AddBoolean(_("Arm start manually"),
             _("Configure whether the start must be armed manually or automatically."),
             false);

  const RoughTimeDelta time_zone =
    CommonInterface::GetComputerSettings().utc_offset;
  AddRoughTime(_("Start open time"), nullptr,
               RoughTime::Invalid(), time_zone);
  AddRoughTime(_("Start close time"), nullptr,
               RoughTime::Invalid(), time_zone);

  AddFloat(_("Start max. speed"),
           _("Maximum speed allowed in start observation zone.  Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"),
           0, 300, 5, false, 0);

  AddFloat(_("Start max. height"),
           _("Maximum height based on start height reference (AGL or MSL) while starting the task.  Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"),
           0, 10000, 25, false, 0);

  static constexpr StaticEnumChoice altitude_reference_list[] = {
    { (unsigned)AltitudeReference::AGL, N_("AGL"),
      N_("Reference is the height above the task point."), },
    { (unsigned)AltitudeReference::MSL, N_("MSL"),
      N_("Reference is altitude above mean sea level."), },
    { 0 }
  };

  AddEnum(_("Start height ref."),
          _("Reference used for start max height rule."),
          altitude_reference_list);

  AddFloat(_("Finish min. height"),
           _("Minimum height based on finish height reference (AGL or MSL) while finishing the task.  Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"),
           0, 10000, 25, false, 0);

  AddEnum(_("Finish height ref."),
          _("Reference used for finish min height rule."),
          altitude_reference_list);

  AddBoolean(_("FAI start / finish rules"),
             _("If enabled, has no max start height or max start speed and requires the minimum height above ground for finish to be greater than 1000m below the start height."),
             false, this);
}

void
TaskPropertiesPanel::ReClick()
{
  dialog.TaskViewClicked();
}

void
TaskPropertiesPanel::Show(const PixelRect &rc)
{
  ordered_task = *ordered_task_pointer;
  orig_taskType = ordered_task->GetFactoryType();

  RefreshView();

  RowFormWidget::Show(rc);
}

bool
TaskPropertiesPanel::Leave()
{
  ReadValues();
  if (orig_taskType != ordered_task->GetFactoryType()) {
    if (ordered_task->GetFactory().MutateTPsToTaskType())
      ordered_task->UpdateGeometry();
  }

  return true;
}
