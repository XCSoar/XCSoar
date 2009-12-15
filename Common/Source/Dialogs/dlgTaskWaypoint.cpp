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
#include "Protection.hpp"
#include "SettingsTask.hpp"
#include "Logger.h"
#include "Screen/Layout.hpp"
#include "Math/FastMath.h"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Compatibility/string.h"
#include "WayPointList.hpp"
#include "Components.hpp"
#include "Task.h"

#include <assert.h>

static unsigned twItemIndex= 0;
static WndForm *wf=NULL;
static int twType = 0; // start, turnpoint, finish
static SETTINGS_TASK settings_task;

static WndFrame *wStart=NULL;
static WndFrame *wTurnpoint=NULL;
static WndFrame *wAATTurnpoint=NULL;
static WndFrame *wFinish=NULL;

static void UpdateCaption(void) {
  TCHAR sTmp[128];
  TCHAR title[128];
  if (task.ValidTaskPoint(twItemIndex)) {
    switch (twType) {
    case 0:
      _stprintf(title, gettext(_T("Start")));
      break;
    case 1:
      _stprintf(title, gettext(_T("Turnpoint")));
      break;
    case 2:
      _stprintf(title, gettext(_T("Finish")));
      break;
    };
    _stprintf(sTmp, _T("%s: %s"), title, task.getWaypoint(twItemIndex).Name);
    wf->SetCaption(sTmp);
  } else {
    wf->SetCaption(gettext(_T("(invalid)")));
  }
}





static void SetValues(bool first=false) {
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFinishLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(_T("Cylinder")));
      dfe->addEnumText(gettext(_T("Line")));
      dfe->addEnumText(gettext(_T("FAI Sector")));
    }
    dfe->Set(settings_task.FinishType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFinishRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(settings_task.FinishRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskStartLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(_T("Cylinder")));
      dfe->addEnumText(gettext(_T("Line")));
      dfe->addEnumText(gettext(_T("FAI Sector")));
    }
    dfe->SetDetachGUI(true); // disable call to OnAATEnabled
    dfe->Set(settings_task.StartType);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskStartRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(settings_task.StartRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFAISector"));
  if (wp) {
    wp->set_visible(settings_task.AATEnabled == 0);
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(_T("Cylinder")));
      dfe->addEnumText(gettext(_T("FAI Sector")));
      dfe->addEnumText(gettext(_T("DAe 0.5/10")));
    }
    dfe->SetDetachGUI(true); // disable call to OnAATEnabled
    dfe->Set(settings_task.SectorType);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskSectorRadius"));
  if (wp) {
    wp->set_visible(settings_task.AATEnabled == 0);
    wp->GetDataField()->SetAsFloat(lround(settings_task.SectorRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoAdvance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(_T("Manual")));
      dfe->addEnumText(gettext(_T("Auto")));
      dfe->addEnumText(gettext(_T("Arm")));
      dfe->addEnumText(gettext(_T("Arm start")));
    }
    dfe->Set(settings_task.AutoAdvance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMinTime"));
  if (wp) {
    wp->set_visible(settings_task.AATEnabled > 0);
    wp->GetDataField()->SetAsFloat(settings_task.AATTaskLength);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpEnableMultipleStartPoints"));
  if (wp) {
    wp->GetDataField()->Set(settings_task.EnableMultipleStartPoints);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAATEnabled"));
  if (wp) {
    bool aw = (settings_task.AATEnabled != 0);
    wp->GetDataField()->Set(aw);
    wp->RefreshDisplay();
  }

  WndButton* wb;
  wb = (WndButton *)wf->FindByName(_T("EditStartPoints"));
  if (wb) {
    wb->set_visible(settings_task.EnableMultipleStartPoints != 0);
  }

}

#define CHECK_CHANGED(a,b) if (a != b) { changed = true; a = b; }
#define CHECK_CHANGEDU(a,b) if ((int)a != b) { changed = true; a = b; }

static void GetWaypointValues(void) {
  WndProperty* wp;
  bool changed = false;

  if (!settings_task.AATEnabled) {
    return;
  }

  if (task.ValidTaskPoint(twItemIndex)) {
    TASK_POINT tp = task.getTaskPoint(twItemIndex);

    short tmp;

    wp = (WndProperty*)wf->FindByName(_T("prpAATType"));
    if (wp) {
      tmp = tp.AATType;
      CHECK_CHANGED(tmp,
                    wp->GetDataField()->GetAsInteger());
      tp.AATType = (AATSectorType_t)tmp;
    }

    wp = (WndProperty*)wf->FindByName(_T("prpAATCircleRadius"));
    if (wp) {
      CHECK_CHANGED(tp.AATCircleRadius,
                    iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY));
    }

    wp = (WndProperty*)wf->FindByName(_T("prpAATSectorRadius"));
    if (wp) {
      CHECK_CHANGED(tp.AATSectorRadius,
                    iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY));
    }

    wp = (WndProperty*)wf->FindByName(_T("prpAATStartRadial"));
    if (wp) {
      CHECK_CHANGED(tp.AATStartRadial,
                    wp->GetDataField()->GetAsInteger());
    }

    wp = (WndProperty*)wf->FindByName(_T("prpAATFinishRadial"));
    if (wp) {
      CHECK_CHANGED(tp.AATFinishRadial,
                    wp->GetDataField()->GetAsInteger());
    }
    task.setTaskPoint(twItemIndex, tp);

    if (changed) {
      task.SetTaskModified();
    }
  }
}


static void SetWaypointValues(bool first=false) {
  WndProperty* wp;

  TASK_POINT tp = task.getTaskPoint(twItemIndex);

  wp = (WndProperty*)wf->FindByName(_T("prpAATType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(_T("Cylinder")));
      dfe->addEnumText(gettext(_T("Sector")));
    }
    dfe->SetDetachGUI(true); // disable call to OnAATEnabled
    dfe->Set(tp.AATType);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAATCircleRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(tp.AATCircleRadius
                                          *DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->set_visible(tp.AATType == 0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAATSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(tp.AATSectorRadius
                                          *DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->set_visible(tp.AATType > 0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAATStartRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(tp.AATStartRadial);
    wp->set_visible(tp.AATType > 0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAATFinishRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(tp.AATFinishRadial);
    wp->set_visible(tp.AATType > 0);
    wp->RefreshDisplay();
  }

}


static void ReadValues(void) {
  WndProperty* wp;
  bool changed = false;

  wp = (WndProperty*)wf->FindByName(_T("prpEnableMultipleStartPoints"));
  if (wp) {
    CHECK_CHANGED(settings_task.EnableMultipleStartPoints,
                  wp->GetDataField()->GetAsBoolean());
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAATEnabled"));
  if (wp) {
    CHECK_CHANGED(settings_task.AATEnabled,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFinishLine"));
  if (wp) {
    unsigned tmp = settings_task.FinishType;
    CHECK_CHANGEDU(tmp,
                  wp->GetDataField()->GetAsInteger());
    settings_task.FinishType = (FinishSectorType_t)tmp;
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFinishRadius"));
  if (wp) {
    CHECK_CHANGED(settings_task.FinishRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskStartLine"));
  if (wp) {
    unsigned tmp = settings_task.StartType;
    CHECK_CHANGEDU(tmp,
                  wp->GetDataField()->GetAsInteger());
    settings_task.StartType = (StartSectorType_t)tmp;
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskStartRadius"));
  if (wp) {
    CHECK_CHANGED(settings_task.StartRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskFAISector"));
  if (wp) {
    unsigned tmp = settings_task.SectorType;
    CHECK_CHANGEDU(tmp,
                  wp->GetDataField()->GetAsInteger());
    settings_task.SectorType = (ASTSectorType_t)tmp;
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTaskSectorRadius"));
  if (wp) {
    CHECK_CHANGED(settings_task.SectorRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoAdvance"));
  if (wp) {
    short tmp= settings_task.AutoAdvance;
    CHECK_CHANGED(tmp,
                  wp->GetDataField()->GetAsInteger());
    settings_task.AutoAdvance = (AutoAdvanceMode_t)tmp;
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMinTime"));
  if (wp) {
    CHECK_CHANGED(settings_task.AATTaskLength,
                  wp->GetDataField()->GetAsInteger());
  }
  if (changed) {
    task.setSettings(settings_task);
    task.SetTaskModified();
  }
}

static void OnAATEnabled(DataField *Sender, DataField::DataAccessKind_t Mode) {
  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    case DataField::daChange:
      ReadValues();
      SetValues();
      GetWaypointValues();
      SetWaypointValues();
    break;
  }
}



static void OnSelectClicked(WindowControl * Sender){
	(void)Sender;
  int res;
  res = dlgWayPointSelect(XCSoarInterface::Basic().Location);
  if (res != -1){
    task.setSelected(res);
    TASK_POINT tp = task.getTaskPoint(twItemIndex);
    if (tp.Index != res) {
      if (logger.CheckDeclaration()) {

	tp.Index = res;
        tp.AATSectorRadius = settings_task.SectorRadius;
        tp.AATCircleRadius = settings_task.SectorRadius;
        tp.AATTargetOffsetRadius = 0.0;
        tp.AATTargetOffsetRadial = 0.0;
        tp.AATTargetLocked = false;
        task.setTaskPoint(twItemIndex, tp);
        task.SetTaskModified();

      }
    }
    UpdateCaption();
  };
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static void OnStartPointClicked(WindowControl * Sender){
	(void)Sender;
  dlgStartPointShowModal();
}


static void OnMoveAfterClicked(WindowControl * Sender){
	(void)Sender;
  task.SwapWaypoint(twItemIndex, XCSoarInterface::SettingsComputer(),
                     XCSoarInterface::Basic());
  SetWaypointValues();
  wf->SetModalResult(mrOK);
}

static void OnMoveBeforeClicked(WindowControl * Sender){
	(void)Sender;
  task.SwapWaypoint(twItemIndex - 1, XCSoarInterface::SettingsComputer(),
                    XCSoarInterface::Basic());
  SetWaypointValues();
  wf->SetModalResult(mrOK);
}

static void OnDetailsClicked(WindowControl * Sender){
  (void)Sender;
//  task.setSelected(task.getWaypointIndex(twItemIndex));
  dlgWayPointDetailsShowModal(way_point);
}

static void OnRemoveClicked(WindowControl * Sender) {
	(void)Sender;
  task.RemoveTaskPoint(twItemIndex, XCSoarInterface::SettingsComputer(),
                       XCSoarInterface::Basic());
  SetWaypointValues();
  wf->SetModalResult(mrOK);
}


static void OnTaskRulesClicked(WindowControl * Sender){
  (void)Sender;
  wf->hide();
  if (dlgTaskRules()) {
    task.SetTaskModified();
  }
  wf->show();
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnSelectClicked),
  DeclareCallBackEntry(OnDetailsClicked),
  DeclareCallBackEntry(OnRemoveClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnStartPointClicked),
  DeclareCallBackEntry(OnMoveAfterClicked),
  DeclareCallBackEntry(OnMoveBeforeClicked),
  DeclareCallBackEntry(OnAATEnabled),
  DeclareCallBackEntry(OnTaskRulesClicked),
  DeclareCallBackEntry(NULL)
};


void dlgTaskWaypointShowModal(int itemindex, int tasktype, bool addonly){
  wf = NULL;

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskWaypoint_L.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_TASKWAYPOINT_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskWaypoint.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_TASKWAYPOINT"));
  }

  twItemIndex = itemindex;
  twType = tasktype;

  if (!wf) return;

  assert(wf!=NULL);
  //  wf->SetKeyDownNotify(FormKeyDown);

  settings_task = task.getSettings();

  wStart     = ((WndFrame *)wf->FindByName(_T("frmStart")));
  wTurnpoint = ((WndFrame *)wf->FindByName(_T("frmTurnpoint")));
  wAATTurnpoint = ((WndFrame *)wf->FindByName(_T("frmAATTurnpoint")));
  wFinish    = ((WndFrame *)wf->FindByName(_T("frmFinish")));

  assert(wStart!=NULL);
  assert(wTurnpoint!=NULL);
  assert(wAATTurnpoint!=NULL);
  assert(wFinish!=NULL);

  WndButton* wb;
  if (addonly) {
    wb = (WndButton *)wf->FindByName(_T("butSelect"));
    if (wb) {
      wb->hide();
    }
    wb = (WndButton *)wf->FindByName(_T("butRemove"));
    if (wb) {
      wb->hide();
    }
    wb = (WndButton *)wf->FindByName(_T("butDetails"));
    if (wb) {
      wb->hide();
    }
    wb = (WndButton *)wf->FindByName(_T("butDown"));
    if (wb) {
      wb->hide();
    }
    wb = (WndButton *)wf->FindByName(_T("butUp"));
    if (wb) {
      wb->hide();
    }
  } else {
    if (!task.ValidTaskPoint(twItemIndex-1)) {
      wb = (WndButton *)wf->FindByName(_T("butUp"));
      if (wb) {
        wb->hide();
      }
    }
    if (!task.ValidTaskPoint(twItemIndex+1)) {
      wb = (WndButton *)wf->FindByName(_T("butDown"));
      if (wb) {
        wb->hide();
      }
    }
  }

  SetWaypointValues(true);

  switch (twType) {
    case 0:
      wStart->show();
      wTurnpoint->hide();
      wAATTurnpoint->hide();
      wFinish->hide();
      break;
    case 1:
      wStart->hide();
      wTurnpoint->set_visible(!settings_task.AATEnabled);
      wAATTurnpoint->set_visible(settings_task.AATEnabled);
      wFinish->hide();
    break;
    case 2:
      wStart->hide();
      wTurnpoint->hide();
      wAATTurnpoint->hide();
      wFinish->show();
    break;
  }

  // set properties...

  SetValues(true);

  UpdateCaption();

  wf->ShowModal();

  // now retrieve changes

  GetWaypointValues();

  ReadValues();

  delete wf;

  wf = NULL;

}
