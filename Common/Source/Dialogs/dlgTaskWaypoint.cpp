/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "InfoBoxLayout.h"
#include "Math/FastMath.h"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Compatibility/string.h"
#include "WayPointList.hpp"
#include "Components.hpp"

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
      _stprintf(title, gettext(TEXT("Start")));
      break;
    case 1:
      _stprintf(title, gettext(TEXT("Turnpoint")));
      break;
    case 2:
      _stprintf(title, gettext(TEXT("Finish")));
      break;
    };
    _stprintf(sTmp, TEXT("%s: %s"), title, task.getWaypoint(twItemIndex).Name);
    wf->SetCaption(sTmp);
  } else {
    wf->SetCaption(gettext(TEXT("(invalid)")));
  }
}





static void SetValues(bool first=false) {
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(TEXT("Cylinder")));
      dfe->addEnumText(gettext(TEXT("Line")));
      dfe->addEnumText(gettext(TEXT("FAI Sector")));
    }
    dfe->Set(settings_task.FinishLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(settings_task.FinishRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(TEXT("Cylinder")));
      dfe->addEnumText(gettext(TEXT("Line")));
      dfe->addEnumText(gettext(TEXT("FAI Sector")));
    }
    dfe->SetDetachGUI(true); // disable call to OnAATEnabled
    dfe->Set(settings_task.StartLine);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(settings_task.StartRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    wp->SetVisible(settings_task.AATEnabled==0);
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(TEXT("Cylinder")));
      dfe->addEnumText(gettext(TEXT("FAI Sector")));
      dfe->addEnumText(gettext(TEXT("DAe 0.5/10")));
    }
    dfe->SetDetachGUI(true); // disable call to OnAATEnabled
    dfe->Set(settings_task.SectorType);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    wp->SetVisible(settings_task.AATEnabled==0);
    wp->GetDataField()->SetAsFloat(lround(settings_task.SectorRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(TEXT("Manual")));
      dfe->addEnumText(gettext(TEXT("Auto")));
      dfe->addEnumText(gettext(TEXT("Arm")));
      dfe->addEnumText(gettext(TEXT("Arm start")));
    }
    dfe->Set(settings_task.AutoAdvance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMinTime"));
  if (wp) {
    wp->SetVisible(settings_task.AATEnabled>0);
    wp->GetDataField()->SetAsFloat(settings_task.AATTaskLength);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableMultipleStartPoints"));
  if (wp) {
    wp->GetDataField()->Set(settings_task.EnableMultipleStartPoints);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    bool aw = (settings_task.AATEnabled != 0);
    wp->GetDataField()->Set(aw);
    wp->RefreshDisplay();
  }

  WndButton* wb;
  wb = (WndButton *)wf->FindByName(TEXT("EditStartPoints"));
  if (wb) {
    wb->SetVisible(settings_task.EnableMultipleStartPoints!=0);
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

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATType"));
    if (wp) {
      CHECK_CHANGED(tp.AATType,
                    wp->GetDataField()->GetAsInteger());
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATCircleRadius"));
    if (wp) {
      CHECK_CHANGED(tp.AATCircleRadius,
                    iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY));
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATSectorRadius"));
    if (wp) {
      CHECK_CHANGED(tp.AATSectorRadius,
                    iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY));
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATStartRadial"));
    if (wp) {
      CHECK_CHANGED(tp.AATStartRadial,
                    wp->GetDataField()->GetAsInteger());
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATFinishRadial"));
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(TEXT("Cylinder")));
      dfe->addEnumText(gettext(TEXT("Sector")));
    }
    dfe->SetDetachGUI(true); // disable call to OnAATEnabled
    dfe->Set(tp.AATType);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATCircleRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(tp.AATCircleRadius
                                          *DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->SetVisible(tp.AATType==0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(tp.AATSectorRadius
                                          *DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->SetVisible(tp.AATType>0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATStartRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(tp.AATStartRadial);
    wp->SetVisible(tp.AATType>0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATFinishRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(tp.AATFinishRadial);
    wp->SetVisible(tp.AATType>0);
    wp->RefreshDisplay();
  }

}


static void ReadValues(void) {
  WndProperty* wp;
  bool changed = false;

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableMultipleStartPoints"));
  if (wp) {
    CHECK_CHANGED(settings_task.EnableMultipleStartPoints,
                  wp->GetDataField()->GetAsBoolean());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    CHECK_CHANGED(settings_task.AATEnabled,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    CHECK_CHANGEDU(settings_task.FinishLine,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    CHECK_CHANGED(settings_task.FinishRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    CHECK_CHANGEDU(settings_task.StartLine,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    CHECK_CHANGED(settings_task.StartRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    CHECK_CHANGEDU(settings_task.SectorType,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    CHECK_CHANGED(settings_task.SectorRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    CHECK_CHANGED(settings_task.AutoAdvance,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMinTime"));
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
  task.SwapWaypoint(twItemIndex, XCSoarInterface::SettingsComputer());
  SetWaypointValues();
  wf->SetModalResult(mrOK);
}

static void OnMoveBeforeClicked(WindowControl * Sender){
	(void)Sender;
  task.SwapWaypoint(twItemIndex-1,XCSoarInterface::SettingsComputer());
  SetWaypointValues();
  wf->SetModalResult(mrOK);
}

static void OnDetailsClicked(WindowControl * Sender){
  (void)Sender;
  task.setSelected(task.getWaypointIndex(twItemIndex));
  PopupWaypointDetails();
}

static void OnRemoveClicked(WindowControl * Sender) {
	(void)Sender;
  task.RemoveTaskPoint(twItemIndex,XCSoarInterface::SettingsComputer());
  SetWaypointValues();
  wf->SetModalResult(mrOK);
}


static void OnTaskRulesClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetVisible(false);
  if (dlgTaskRules()) {
    task.SetTaskModified();
  }
  wf->SetVisible(true);
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

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgTaskWaypoint_L.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_TASKWAYPOINT_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgTaskWaypoint.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_TASKWAYPOINT"));
  }

  twItemIndex = itemindex;
  twType = tasktype;

  if (!wf) return;

  assert(wf!=NULL);
  //  wf->SetKeyDownNotify(FormKeyDown);

  settings_task = task.getSettings();

  wStart     = ((WndFrame *)wf->FindByName(TEXT("frmStart")));
  wTurnpoint = ((WndFrame *)wf->FindByName(TEXT("frmTurnpoint")));
  wAATTurnpoint = ((WndFrame *)wf->FindByName(TEXT("frmAATTurnpoint")));
  wFinish    = ((WndFrame *)wf->FindByName(TEXT("frmFinish")));

  assert(wStart!=NULL);
  assert(wTurnpoint!=NULL);
  assert(wAATTurnpoint!=NULL);
  assert(wFinish!=NULL);

  WndButton* wb;
  if (addonly) {
    wb = (WndButton *)wf->FindByName(TEXT("butSelect"));
    if (wb) {
      wb->SetVisible(false);
    }
    wb = (WndButton *)wf->FindByName(TEXT("butRemove"));
    if (wb) {
      wb->SetVisible(false);
    }
    wb = (WndButton *)wf->FindByName(TEXT("butDetails"));
    if (wb) {
      wb->SetVisible(false);
    }
    wb = (WndButton *)wf->FindByName(TEXT("butDown"));
    if (wb) {
      wb->SetVisible(false);
    }
    wb = (WndButton *)wf->FindByName(TEXT("butUp"));
    if (wb) {
      wb->SetVisible(false);
    }
  } else {
    if (!task.ValidTaskPoint(twItemIndex-1)) {
      wb = (WndButton *)wf->FindByName(TEXT("butUp"));
      if (wb) {
        wb->SetVisible(false);
      }
    }
    if (!task.ValidTaskPoint(twItemIndex+1)) {
      wb = (WndButton *)wf->FindByName(TEXT("butDown"));
      if (wb) {
        wb->SetVisible(false);
      }
    }
  }

  SetWaypointValues(true);

  switch (twType) {
    case 0:
      wStart->SetVisible(1);
      wTurnpoint->SetVisible(0);
      wAATTurnpoint->SetVisible(0);
      wFinish->SetVisible(0);
      break;
    case 1:
      wStart->SetVisible(0);
      if (settings_task.AATEnabled) {
	wTurnpoint->SetVisible(0);
	wAATTurnpoint->SetVisible(1);
      } else {
	wTurnpoint->SetVisible(1);
	wAATTurnpoint->SetVisible(0);
      }
      wFinish->SetVisible(0);
    break;
    case 2:
      wStart->SetVisible(0);
      wTurnpoint->SetVisible(0);
      wAATTurnpoint->SetVisible(0);
      wFinish->SetVisible(1);
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
