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

#include <assert.h>

static SingleWindow *parent_window;
static WndForm *wf = NULL;
static OrderedTask* ordered_task = NULL;
static bool* task_changed = NULL;
static OrderedTask** ordered_task_pointer = NULL;

static void 
InitView()
{
  WndProperty* wp;
  wp = ((WndProperty*)wf->FindByName(_T("prpStartMaxSpeed")));
  if (wp)
    wp->GetDataField()->SetUnits(Units::GetSpeedName());

  wp = ((WndProperty*)wf->FindByName(_T("prpStartMaxHeight")));
  if (wp)
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());

  wp = ((WndProperty*)wf->FindByName(_T("prpFinishMinHeight")));
  if (wp)
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());

  wp = (WndProperty*)wf->FindByName(_T("prpStartHeightRef"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("AGL"));
    dfe->addEnumText(_("MSL"));
  }
}

static void 
RefreshView()
{
  WndProperty* wp;
  TaskBehaviour::Factory_t ftype = ordered_task->get_factory_type();
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();

  bool aat_types = (ftype == TaskBehaviour::FACTORY_AAT);
  bool racing_types = (ftype == TaskBehaviour::FACTORY_RT) || aat_types;

  wp = ((WndProperty*)wf->FindByName(_T("prpMinTime")));
  if (wp) {
    wp->set_visible(aat_types);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(p.aat_min_time/60);
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpFAIFinishHeight")));
  if (wp) {
    DataFieldBoolean &df = *(DataFieldBoolean *)wp->GetDataField();
    df.SetAsBoolean(p.fai_finish);
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpStartMaxSpeed")));
  if (wp) {
    wp->set_visible(racing_types);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserSpeed(p.start_max_speed));
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpStartMaxHeight")));
  if (wp) {
    wp->set_visible(racing_types);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserAltitude(fixed(p.start_max_height)));
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpFinishMinHeight")));
  if (wp) {
    wp->set_visible(!p.fai_finish);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserAltitude(fixed(p.finish_min_height)));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpStartHeightRef"));
  if (wp) {
    wp->set_visible(racing_types);
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(p.start_max_height_ref);
    wp->RefreshDisplay();
  }

  WndButton* wb;
  wb = ((WndButton*)wf->FindByName(_T("butType")));
  if (wb)
    wb->SetCaption(OrderedTaskFactoryName(ordered_task->get_factory_type()));

  // fixed aat_min_time
  // finish_min_height
}


static void 
ReadValues()
{
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();

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

  unsigned height_ref = GetFormValueInteger(*wf, _T("prpStartHeightRef"));
  if (height_ref != p.start_max_height_ref) {
    p.start_max_height_ref = height_ref;
    *task_changed = true;
  }
}

void
pnlTaskProperties::OnTypeClicked(WndButton &Sender)
{
  (void)Sender;

  TaskBehaviour::Factory_t new_type = TaskBehaviour::FACTORY_FAI_GENERAL;

 if (dlgTaskTypeShowModal(*parent_window, &ordered_task, new_type)) {
   if (new_type != ordered_task->get_factory_type()) {
     ordered_task->set_factory(new_type);
     if (new_type != TaskBehaviour::FACTORY_MIXED)
       ordered_task->get_factory().mutate_tps_to_task_type();

     *task_changed = true;
   }
 }

 RefreshView();
}

bool
pnlTaskProperties::OnTabPreShow(unsigned EventType)
{
  ordered_task = *ordered_task_pointer;
  RefreshView();
  return true;
}

bool
pnlTaskProperties::OnTabPreHide()
{
  ReadValues();
  return true;
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

Window*
pnlTaskProperties::Load(SingleWindow &parent, TabBarControl* wTabBar,
                        WndForm* _wf, OrderedTask** task, bool* _task_modified)
{
  parent_window = &parent;
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

  assert(wProp);

  InitView();

  return wProp;
}
