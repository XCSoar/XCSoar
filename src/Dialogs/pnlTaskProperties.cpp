/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Float.hpp"

#include "Dialogs/dlgTaskHelpers.hpp"
#include "Dialogs/dlgTaskManager.hpp"

#include "Task/Tasks/OrderedTask.hpp"

#include <assert.h>
#include <stdio.h>

static WndForm *wf = NULL;
static OrderedTask* ordered_task = NULL;
static WndOwnerDrawFrame* wTaskView = NULL;
static bool* task_changed = NULL;
static OrderedTask** ordered_task_pointer = NULL;
static TaskBehaviour::Factory_t orig_taskType =
    TaskBehaviour::FACTORY_RT;

static void 
InitView()
{
  WndProperty* wp;

  static const StaticEnumChoice start_max_height_ref_list[] = {
    { hrAGL, N_("AGL"), N_("Reference AGL for start maximum height rule (above start point)") },
    { hrMSL, N_("MSL"), N_("Reference MSL for start maximum height rule (above sea level)") },
    { 0 }
  };
  LoadFormProperty(*wf, _T("prpStartHeightRef"), start_max_height_ref_list, hrAGL);

  static const StaticEnumChoice finish_min_height_ref_list[] = {
    { hrAGL, N_("AGL"), N_("Reference AGL for finish minimum height rule (above finish point)") },
    { hrMSL, N_("MSL"), N_("Reference MSL for finish minimum height rule (above sea level)") },
    { 0 }
  };
  LoadFormProperty(*wf, _T("prpFinishHeightRef"), finish_min_height_ref_list, hrAGL);

  wp = (WndProperty*)wf->FindByName(_T("prpTaskType"));
  if (wp) {
    const std::vector<TaskBehaviour::Factory_t> factory_types =
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

static void 
RefreshView()
{
  const TaskBehaviour::Factory_t ftype = ordered_task->get_factory_type();
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();

  bool aat_types = (ftype == TaskBehaviour::FACTORY_AAT);
  bool fai_start_finish = p.fai_finish;

  LoadFormProperty(*wf, _T("prpTaskType"),(int)ftype);

  ShowFormControl(*wf, _T("prpMinTime"), aat_types);
  LoadFormProperty(*wf, _T("prpMinTime"), p.aat_min_time / 60);

  LoadFormProperty(*wf, _T("prpFAIFinishHeight"), p.fai_finish);

  ShowFormControl(*wf, _T("prpStartMaxSpeed"), !fai_start_finish);
  LoadFormProperty(*wf, _T("prpStartMaxSpeed"),
                   ugHorizontalSpeed, p.start_max_speed);

  ShowFormControl(*wf, _T("prpStartMaxHeight"), !fai_start_finish);
  LoadFormProperty(*wf, _T("prpStartMaxHeight"),
                   ugAltitude, p.start_max_height);

  ShowFormControl(*wf, _T("prpFinishMinHeight"), !fai_start_finish);
  LoadFormProperty(*wf, _T("prpFinishMinHeight"),
                   ugAltitude, p.finish_min_height);

  ShowFormControl(*wf, _T("prpStartHeightRef"), !fai_start_finish);
  LoadFormProperty(*wf, _T("prpStartHeightRef"), p.start_max_height_ref);

  ShowFormControl(*wf, _T("prpFinishHeightRef"), !fai_start_finish);
  LoadFormProperty(*wf, _T("prpFinishHeightRef"), p.finish_min_height_ref);

  wTaskView->invalidate();

  // fixed aat_min_time
  // finish_min_height
}


static void 
ReadValues()
{
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();

  TaskBehaviour::Factory_t newtype = ordered_task->get_factory_type();
  *task_changed |= SaveFormPropertyEnum(*wf, _T("prpTaskType"), newtype);

  fixed min_time = GetFormValueFixed(*wf, _T("prpMinTime")) * 60;
  if (min_time != p.aat_min_time) {
    p.aat_min_time = min_time;
    *task_changed = true;
  }

  unsigned max_height =
    iround(Units::ToSysAltitude(GetFormValueFixed(*wf, _T("prpStartMaxHeight"))));
  if (max_height != p.start_max_height) {
    p.start_max_height = max_height;
    *task_changed = true;
  }

  fixed max_speed =
    Units::ToSysSpeed(GetFormValueFixed(*wf, _T("prpStartMaxSpeed")));
  if (max_speed != p.start_max_speed) {
    p.start_max_speed = max_speed;
    *task_changed = true;
  }

  unsigned min_height =
    iround(Units::ToSysAltitude(GetFormValueFixed(*wf, _T("prpFinishMinHeight"))));
  if (min_height != p.finish_min_height) {
    p.finish_min_height = min_height;
    *task_changed = true;
  }

  HeightReferenceType height_ref_start = (HeightReferenceType)
      GetFormValueInteger(*wf, _T("prpStartHeightRef"));
  if (height_ref_start != p.start_max_height_ref) {
    p.start_max_height_ref = height_ref_start;
    *task_changed = true;
  }

  HeightReferenceType height_ref_finish = (HeightReferenceType)
      GetFormValueInteger(*wf, _T("prpFinishHeightRef"));
  if (height_ref_finish != p.finish_min_height_ref) {
    p.finish_min_height_ref = height_ref_finish;
    *task_changed = true;
  }
}

bool
pnlTaskProperties::OnTabPreShow(TabBarControl::EventType EventType)
{
  ordered_task = *ordered_task_pointer;
  orig_taskType = ordered_task->get_factory_type();
  LoadFormProperty(*wf, _T("prpTaskType"),
      (unsigned)orig_taskType);
  dlgTaskManager::TaskViewRestore(wTaskView);
  RefreshView();
  return true;
}

bool
pnlTaskProperties::OnTabPreHide()
{
  ReadValues();
  if (orig_taskType != ordered_task->get_factory_type())
    ordered_task->GetFactory().mutate_tps_to_task_type();

  return true;
}

void
pnlTaskProperties::OnTabReClick()
{
  dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
}

void
pnlTaskProperties::OnFAIFinishHeightData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();
  if (Mode == DataField::daChange) {
    bool newvalue = ((DataFieldBoolean*)Sender)->GetAsBoolean();
    if (newvalue != p.fai_finish) {
      p.fai_finish = newvalue;
      *task_changed = true;
      RefreshView();
    }
  }
}


void
pnlTaskProperties::OnTaskTypeData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  if (Mode == DataField::daChange) {
    const DataFieldEnum &df = *(DataFieldEnum *)Sender;
    const TaskBehaviour::Factory_t newtype =
       (TaskBehaviour::Factory_t)df.GetAsInteger();
    if (newtype != ordered_task->get_factory_type()) {
      ReadValues();
      ordered_task->SetFactory(newtype);
      *task_changed =true;
      RefreshView();
    }
  }
}

Window*
pnlTaskProperties::Load(SingleWindow &parent, TabBarControl* wTabBar,
                        WndForm* _wf, OrderedTask** task, bool* _task_modified)
{
  ordered_task_pointer = task;
  ordered_task = *ordered_task_pointer;;

  assert(_task_modified);
  task_changed = _task_modified;

  assert(_wf);
  wf = _wf;

  Window *wProp =
      LoadWindow(dlgTaskManager::CallBackTable, wf, *wTabBar,
                 Layout::landscape ?
                 _T("IDR_XML_TASKPROPERTIES_L") : _T("IDR_XML_TASKPROPERTIES"));

  wTaskView = (WndOwnerDrawFrame*)wf->FindByName(_T("frmTaskViewProperties"));
  assert(wTaskView != NULL);
  wTaskView->SetOnMouseDownNotify(dlgTaskManager::OnTaskViewClick);

  assert(wProp);

  InitView();

  return wProp;
}
