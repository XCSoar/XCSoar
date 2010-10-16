/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Dialogs/dlgTaskHelpers.hpp"

#include <assert.h>

static SingleWindow *parent_window;
static WndForm *wf=NULL;
static OrderedTask* ordered_task= NULL;
static bool task_changed = false;

static void 
InitView()
{
  WndProperty* wp;
  wp = ((WndProperty*)wf->FindByName(_T("prpStartMaxSpeed")));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetSpeedName());
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpStartMaxHeight")));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpFinishMinHeight")));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
  }

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
  OrderedTask::Factory_t ftype = ordered_task->get_factory_type();
  OrderedTaskBehaviour &p = ordered_task->get_ordered_task_behaviour();

  bool fai_types = (ftype == OrderedTask::FACTORY_FAI_GENERAL) ||
    (ftype == OrderedTask::FACTORY_FAI_TRIANGLE) ||
    (ftype == OrderedTask::FACTORY_FAI_OR) ||
    (ftype == OrderedTask::FACTORY_FAI_GOAL);
  bool aat_types = 
    (ftype == OrderedTask::FACTORY_AAT) ||
    (ftype == OrderedTask::FACTORY_MIXED);
  bool racing_types = 
    (ftype == OrderedTask::FACTORY_RT) || aat_types;

  wp = ((WndProperty*)wf->FindByName(_T("prpTaskScored")));
  if (wp) {
    wp->set_visible(fai_types || racing_types);
    wp->GetDataField()->SetAsBoolean(p.task_scored);
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpMinTime")));
  if (wp) {
    wp->set_visible(aat_types);
    wp->GetDataField()->SetAsFloat(p.aat_min_time/60);
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpFAIFinishHeight")));
  if (wp) {
    wp->set_visible(fai_types);
    wp->GetDataField()->SetAsBoolean(p.fai_finish);
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpStartMaxSpeed")));
  if (wp) {
    wp->set_visible(racing_types);
    wp->GetDataField()->SetAsFloat(Units::ToUserSpeed(p.start_max_speed));
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpStartMaxHeight")));
  if (wp) {
    wp->set_visible(racing_types);
    wp->GetDataField()->SetAsFloat(Units::ToUserAltitude(fixed(p.start_max_height)));
    wp->RefreshDisplay();
  }

  wp = ((WndProperty*)wf->FindByName(_T("prpFinishMinHeight")));
  if (wp) {
    wp->set_visible(racing_types);
    wp->GetDataField()->SetAsFloat(Units::ToUserAltitude(fixed(p.finish_min_height)));
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
  if (wb) {
    wb->SetCaption(OrderedTaskFactoryName(ordered_task->get_factory_type()));
  }

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
    task_changed = true;
  }

  bool finish_height = GetFormValueBoolean(*wf, _T("prpFAIFinishHeight"));
  if (finish_height != p.fai_finish) {
    p.fai_finish = finish_height;
    task_changed = true;
  }

  unsigned max_height =
    iround(Units::ToSysAltitude(GetFormValueFixed(*wf, _T("prpStartMaxHeight"))));
  if (max_height != p.start_max_height) {
    p.start_max_height = max_height;
    task_changed = true;
  }

  fixed max_speed =
    Units::ToSysSpeed(GetFormValueFixed(*wf, _T("prpStartMaxSpeed")));
  if (max_speed != p.start_max_speed) {
    p.start_max_speed = max_speed;
    task_changed = true;
  }

  unsigned min_height =
    iround(Units::ToSysAltitude(GetFormValueFixed(*wf, _T("prpFinishMinHeight"))));
  if (min_height != p.finish_min_height) {
    p.finish_min_height = max_height;
    task_changed = true;
  }

  unsigned height_ref = GetFormValueInteger(*wf, _T("prpStartHeightRef"));
  if (height_ref != p.start_max_height_ref) {
    p.start_max_height_ref = height_ref;
    task_changed = true;
  }
}


static void OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void OnTypeClicked(WindowControl * Sender)
{
  (void)Sender;
  dlgTaskTypeShowModal(*parent_window, &ordered_task);
  RefreshView();
}

static CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnTypeClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgTaskPropertiesShowModal(SingleWindow &parent, OrderedTask** task)
{
  parent_window = &parent;
  ordered_task = *task;
  task_changed = false;

  wf = NULL;

  if (Layout::landscape) {
    wf = LoadDialog(CallBackTable,
                        parent,
                        _T("IDR_XML_TASKPROPERTIES_L"));
  } else {
    wf = LoadDialog(CallBackTable,
                        parent,
                        _T("IDR_XML_TASKPROPERTIES"));
  }

  if (!wf) return false;
  assert(wf!=NULL);

  InitView();

  RefreshView();

  wf->ShowModal();

  ReadValues();

  delete wf;
  wf = NULL;

  if (*task != ordered_task) {
    *task = ordered_task;
    return true;
  } else {
    return task_changed;
  }
}
