/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Screen/Layout.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Float.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Form/Edit.hpp"
#include "Form/Draw.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"

enum Controls {
  MIN_TIME,
  START_MAX_SPEED,
  START_MAX_HEIGHT,
  START_HEIGHT_REF,
  FINISH_MIN_HEIGHT,
  FINISH_HEIGHT_REF,
  FAI_FINISH_HEIGHT,
  TASK_TYPE,
};

void
TaskPropertiesPanel::RefreshView()
{
  const TaskFactoryType ftype = ordered_task->GetFactoryType();
  const OrderedTaskBehaviour &p = ordered_task->GetOrderedTaskBehaviour();

  bool aat_types = (ftype == TaskFactoryType::AAT);
  bool fai_start_finish = p.fai_finish;

  SetRowVisible(MIN_TIME, aat_types);
  LoadValueTime(MIN_TIME, (int)p.aat_min_time);

  SetRowVisible(START_MAX_SPEED, !fai_start_finish);
  LoadValue(START_MAX_SPEED, p.start_max_speed, UnitGroup::HORIZONTAL_SPEED);

  SetRowVisible(START_MAX_HEIGHT, !fai_start_finish);
  LoadValue(START_MAX_HEIGHT, fixed(p.start_max_height), UnitGroup::ALTITUDE);

  SetRowVisible(START_HEIGHT_REF, !fai_start_finish);
  LoadValueEnum(START_HEIGHT_REF, p.start_max_height_ref);

  SetRowVisible(FINISH_MIN_HEIGHT, !fai_start_finish);
  LoadValue(FINISH_MIN_HEIGHT, fixed(p.finish_min_height),
            UnitGroup::ALTITUDE);

  SetRowVisible(FINISH_HEIGHT_REF, !fai_start_finish);
  LoadValueEnum(FINISH_HEIGHT_REF, p.finish_min_height_ref);

  LoadValue(FAI_FINISH_HEIGHT, p.fai_finish);

  LoadValueEnum(TASK_TYPE, ftype);

  if (wTaskView != NULL)
    wTaskView->Invalidate();

  // fixed aat_min_time
  // finish_min_height
}

void
TaskPropertiesPanel::ReadValues()
{
  OrderedTaskBehaviour &p = ordered_task->GetOrderedTaskBehaviour();

  TaskFactoryType newtype = ordered_task->GetFactoryType();
  *task_changed |= SaveValueEnum(TASK_TYPE, newtype);

  int min_time = GetValueInteger(MIN_TIME);
  if (min_time != (int)p.aat_min_time) {
    p.aat_min_time = fixed(min_time);
    *task_changed = true;
  }

  fixed max_speed = Units::ToSysSpeed(GetValueFloat(START_MAX_SPEED));
  if (max_speed != p.start_max_speed) {
    p.start_max_speed = max_speed;
    *task_changed = true;
  }

  unsigned max_height =
    iround(Units::ToSysAltitude(GetValueFloat(START_MAX_HEIGHT)));
  if (max_height != p.start_max_height) {
    p.start_max_height = max_height;
    *task_changed = true;
  }

  *task_changed |= SaveValueEnum(START_HEIGHT_REF, p.start_max_height_ref);

  unsigned min_height =
    iround(Units::ToSysAltitude(GetValueFloat(FINISH_MIN_HEIGHT)));
  if (min_height != p.finish_min_height) {
    p.finish_min_height = min_height;
    *task_changed = true;
  }

  *task_changed |= SaveValueEnum(FINISH_HEIGHT_REF, p.finish_min_height_ref);
}

void
TaskPropertiesPanel::OnFAIFinishHeightChange(DataFieldBoolean &df)
{
  OrderedTaskBehaviour &p = ordered_task->GetOrderedTaskBehaviour();
  bool newvalue = df.GetAsBoolean();
  if (newvalue != p.fai_finish) {
    p.fai_finish = newvalue;
    *task_changed = true;
    RefreshView();
  }
}

void
TaskPropertiesPanel::OnTaskTypeChange(DataFieldEnum &df)
{
  const TaskFactoryType newtype =
    (TaskFactoryType)df.GetAsInteger();
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
  AddTime(_("AAT min. time"), _("Minimum AAT task time in minutes."),
          0, 36000, 60, 180);

  AddFloat(_("Start max. speed"),
           _("Maximum speed allowed in start observation zone.  Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"),
           fixed_zero, fixed(300), fixed(5), false, fixed_zero);

  AddFloat(_("Start max. height"),
           _("Maximum height based on start height reference (AGL or MSL) while starting the task.  Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"),
           fixed_zero, fixed(10000), fixed(25), false, fixed_zero);

  static constexpr StaticEnumChoice start_max_height_ref_list[] = {
    { (unsigned)HeightReferenceType::AGL, N_("AGL"), N_("Reference AGL for start maximum height rule (above start point).") },
    { (unsigned)HeightReferenceType::MSL, N_("MSL"), N_("Reference MSL for start maximum height rule (above sea level).") },
    { 0 }
  };

  AddEnum(_("Start height ref."),
          _("Reference used for start max height rule\n"
            "[MSL] Reference is altitude above mean sea level\n"
            "[AGL] Reference is the height above the start point"),
          start_max_height_ref_list);

  AddFloat(_("Finish min. height"),
           _("Minimum height based on finish height reference (AGL or MSL) while finishing the task.  Set to 0 for no limit."),
           _T("%.0f %s"), _T("%.0f"),
           fixed_zero, fixed(10000), fixed(25), false, fixed_zero);

  static constexpr StaticEnumChoice finish_min_height_ref_list[] = {
    { (unsigned)HeightReferenceType::AGL, N_("AGL"), N_("Reference AGL for finish minimum height rule (above finish point).") },
    { (unsigned)HeightReferenceType::MSL, N_("MSL"), N_("Reference MSL for finish minimum height rule (above sea level).") },
    { 0 }
  };
  AddEnum(_("Finish height ref."),
          _("Reference used for finish min height rule\n"
            "[MSL] Reference is altitude above mean sea level\n"
            "[AGL] Reference is the height above the finish point"),
          finish_min_height_ref_list);

  AddBoolean(_("FAI start / finish rules"),
             _("If enabled, has no max start height or max start speed and requires the minimum height above ground for finish to be greater than 1000m below the start height."),
             false, this);

  DataFieldEnum *dfe = new DataFieldEnum(NULL);
  dfe->SetListener(this);
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
}

void
TaskPropertiesPanel::ReClick()
{
  if (wTaskView != NULL)
    dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
}

void
TaskPropertiesPanel::Show(const PixelRect &rc)
{
  if (wTaskView != NULL)
    wTaskView->Show();

  ordered_task = *ordered_task_pointer;
  orig_taskType = ordered_task->GetFactoryType();

  RefreshView();

  RowFormWidget::Show(rc);
}

void
TaskPropertiesPanel::Hide()
{
  if (wTaskView != NULL)
    dlgTaskManager::ResetTaskView(wTaskView);

  RowFormWidget::Hide();
}

bool
TaskPropertiesPanel::Leave()
{
  ReadValues();
  if (orig_taskType != ordered_task->GetFactoryType())
    ordered_task->GetFactory().MutateTPsToTaskType();

  return true;
}
