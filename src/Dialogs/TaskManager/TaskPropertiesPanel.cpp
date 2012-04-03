/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Form/Util.hpp"
#include "Form/Edit.hpp"
#include "Form/Draw.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <stdio.h>

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TaskPropertiesPanel *instance;

void
TaskPropertiesPanel::InitView()
{
  WndProperty* wp;

  static gcc_constexpr_data StaticEnumChoice start_max_height_ref_list[] = {
    { (unsigned)HeightReferenceType::AGL, N_("AGL"), N_("Reference AGL for start maximum height rule (above start point).") },
    { (unsigned)HeightReferenceType::MSL, N_("MSL"), N_("Reference MSL for start maximum height rule (above sea level).") },
    { 0 }
  };
  LoadFormProperty(form, _T("prpStartHeightRef"), start_max_height_ref_list,
                   (unsigned)HeightReferenceType::AGL);

  static gcc_constexpr_data StaticEnumChoice finish_min_height_ref_list[] = {
    { (unsigned)HeightReferenceType::AGL, N_("AGL"), N_("Reference AGL for finish minimum height rule (above finish point).") },
    { (unsigned)HeightReferenceType::MSL, N_("MSL"), N_("Reference MSL for finish minimum height rule (above sea level).") },
    { 0 }
  };
  LoadFormProperty(form, _T("prpFinishHeightRef"), finish_min_height_ref_list,
                   (unsigned)HeightReferenceType::AGL);

  wp = (WndProperty *)form.FindByName(_T("prpTaskType"));
  if (wp) {
    const std::vector<TaskFactoryType> factory_types =
        ordered_task->GetFactoryTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < factory_types.size(); i++) {
      dfe->addEnumText(OrderedTaskFactoryName(factory_types[i]),
          (unsigned)factory_types[i], OrderedTaskFactoryDescription(
              factory_types[i]));
      if (factory_types[i] == ordered_task->get_factory_type())
        dfe->Set((unsigned)factory_types[i]);
    }
    wp->RefreshDisplay();
  }
}

void
TaskPropertiesPanel::RefreshView()
{
  const TaskFactoryType ftype = ordered_task->get_factory_type();
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();

  bool aat_types = (ftype == TaskFactoryType::AAT);
  bool fai_start_finish = p.fai_finish;

  LoadFormProperty(form, _T("prpTaskType"),(int)ftype);

  ShowFormControl(form, _T("prpMinTime"), aat_types);
  LoadFormProperty(form, _T("prpMinTime"), (int)p.aat_min_time);

  LoadFormProperty(form, _T("prpFAIFinishHeight"), p.fai_finish);

  ShowFormControl(form, _T("prpStartMaxSpeed"), !fai_start_finish);
  LoadFormProperty(form, _T("prpStartMaxSpeed"),
                   UnitGroup::HORIZONTAL_SPEED, p.start_max_speed);

  ShowFormControl(form, _T("prpStartMaxHeight"), !fai_start_finish);
  LoadFormProperty(form, _T("prpStartMaxHeight"),
                   UnitGroup::ALTITUDE, p.start_max_height);

  ShowFormControl(form, _T("prpFinishMinHeight"), !fai_start_finish);
  LoadFormProperty(form, _T("prpFinishMinHeight"),
                   UnitGroup::ALTITUDE, p.finish_min_height);

  ShowFormControl(form, _T("prpStartHeightRef"), !fai_start_finish);
  LoadFormProperty(form, _T("prpStartHeightRef"), (unsigned)p.start_max_height_ref);

  ShowFormControl(form, _T("prpFinishHeightRef"), !fai_start_finish);
  LoadFormProperty(form, _T("prpFinishHeightRef"), (unsigned)p.finish_min_height_ref);

  wTaskView->Invalidate();

  // fixed aat_min_time
  // finish_min_height
}

void
TaskPropertiesPanel::ReadValues()
{
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();

  TaskFactoryType newtype = ordered_task->get_factory_type();
  *task_changed |= SaveFormPropertyEnum(form, _T("prpTaskType"), newtype);

  int min_time = GetFormValueInteger(form, _T("prpMinTime"));
  if (min_time != (int)p.aat_min_time) {
    p.aat_min_time = fixed(min_time);
    *task_changed = true;
  }

  unsigned max_height =
    iround(Units::ToSysAltitude(GetFormValueFixed(form, _T("prpStartMaxHeight"))));
  if (max_height != p.start_max_height) {
    p.start_max_height = max_height;
    *task_changed = true;
  }

  fixed max_speed =
    Units::ToSysSpeed(GetFormValueFixed(form, _T("prpStartMaxSpeed")));
  if (max_speed != p.start_max_speed) {
    p.start_max_speed = max_speed;
    *task_changed = true;
  }

  unsigned min_height =
    iround(Units::ToSysAltitude(GetFormValueFixed(form, _T("prpFinishMinHeight"))));
  if (min_height != p.finish_min_height) {
    p.finish_min_height = min_height;
    *task_changed = true;
  }

  HeightReferenceType height_ref_start = (HeightReferenceType)
      GetFormValueInteger(form, _T("prpStartHeightRef"));
  if (height_ref_start != p.start_max_height_ref) {
    p.start_max_height_ref = height_ref_start;
    *task_changed = true;
  }

  HeightReferenceType height_ref_finish = (HeightReferenceType)
      GetFormValueInteger(form, _T("prpFinishHeightRef"));
  if (height_ref_finish != p.finish_min_height_ref) {
    p.finish_min_height_ref = height_ref_finish;
    *task_changed = true;
  }
}

void
TaskPropertiesPanel::OnFAIFinishHeightChange(DataFieldBoolean &df)
{
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();
  bool newvalue = df.GetAsBoolean();
  if (newvalue != p.fai_finish) {
    p.fai_finish = newvalue;
    *task_changed = true;
    RefreshView();
  }
}

static void
OnFAIFinishHeightData(DataField *Sender, DataField::DataAccessMode Mode)
{
  if (Mode == DataField::daChange)
    instance->OnFAIFinishHeightChange(*(DataFieldBoolean*)Sender);
}

void
TaskPropertiesPanel::OnTaskTypeChange(DataFieldEnum &df)
{
  const TaskFactoryType newtype =
    (TaskFactoryType)df.GetAsInteger();
  if (newtype != ordered_task->get_factory_type()) {
    ReadValues();
    ordered_task->SetFactory(newtype);
    *task_changed =true;
    RefreshView();
  }
}

static void
OnTaskTypeData(DataField *Sender, DataField::DataAccessMode Mode)
{
  if (Mode == DataField::daChange)
    instance->OnTaskTypeChange(*(DataFieldEnum *)Sender);
}

static gcc_constexpr_data CallBackTableEntry task_properties_callbacks[] = {
  DeclareCallBackEntry(dlgTaskManager::OnTaskPaint),

  DeclareCallBackEntry(OnTaskTypeData),
  DeclareCallBackEntry(OnFAIFinishHeightData),

  DeclareCallBackEntry(NULL)
};

void
TaskPropertiesPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;

  LoadWindow(task_properties_callbacks, parent,
             Layout::landscape
             ? _T("IDR_XML_TASKPROPERTIES_L") : _T("IDR_XML_TASKPROPERTIES"));

  wTaskView = (WndOwnerDrawFrame*)form.FindByName(_T("frmTaskViewProperties"));
  assert(wTaskView != NULL);
  wTaskView->SetOnMouseDownNotify(dlgTaskManager::OnTaskViewClick);

  InitView();
}

void
TaskPropertiesPanel::ReClick()
{
  dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
}

void
TaskPropertiesPanel::Show(const PixelRect &rc)
{
  ordered_task = *ordered_task_pointer;
  orig_taskType = ordered_task->get_factory_type();

  LoadFormProperty(form, _T("prpTaskType"), (unsigned)orig_taskType);
  dlgTaskManager::TaskViewRestore(wTaskView);
  RefreshView();

  XMLWidget::Show(rc);
}

bool
TaskPropertiesPanel::Leave()
{
  ReadValues();
  if (orig_taskType != ordered_task->get_factory_type())
    ordered_task->GetFactory().MutateTPsToTaskType();

  return true;
}

void
TaskPropertiesPanel::Hide()
{
  XMLWidget::Hide();
}
