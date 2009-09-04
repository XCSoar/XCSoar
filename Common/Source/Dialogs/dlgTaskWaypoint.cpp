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

#include "XCSoar.h"
#include "Protection.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "SettingsTask.hpp"
#include "Dialogs/dlgTools.h"
#include "Logger.h"
#include "InfoBoxLayout.h"
#include "Math/FastMath.h"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Compatibility/string.h"

#include <assert.h>

static int twItemIndex= 0;
static WndForm *wf=NULL;
static int twType = 0; // start, turnpoint, finish

static WndFrame *wStart=NULL;
static WndFrame *wTurnpoint=NULL;
static WndFrame *wAATTurnpoint=NULL;
static WndFrame *wFinish=NULL;

static void UpdateCaption(void) {
  TCHAR sTmp[128];
  TCHAR title[128];
  if (ValidTaskPoint(twItemIndex)) {
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
    _stprintf(sTmp, TEXT("%s: %s"), title,
              WayPointList[Task[twItemIndex].Index].Name);
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
    dfe->Set(FinishLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(FinishRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
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
    dfe->Set(StartLine);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(StartRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    wp->SetVisible(AATEnabled==0);
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(TEXT("Cylinder")));
      dfe->addEnumText(gettext(TEXT("FAI Sector")));
      dfe->addEnumText(gettext(TEXT("DAe 0.5/10")));
    }
    dfe->SetDetachGUI(true); // disable call to OnAATEnabled
    dfe->Set(SectorType);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    wp->SetVisible(AATEnabled==0);
    wp->GetDataField()->SetAsFloat(lround(SectorRadius*DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
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
    dfe->Set(AutoAdvance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMinTime"));
  if (wp) {
    wp->SetVisible(AATEnabled>0);
    wp->GetDataField()->SetAsFloat(AATTaskLength);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableMultipleStartPoints"));
  if (wp) {
    wp->GetDataField()->Set(EnableMultipleStartPoints);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    bool aw = (AATEnabled != 0);
    wp->GetDataField()->Set(aw);
    wp->RefreshDisplay();
  }

  WndButton* wb;
  wb = (WndButton *)wf->FindByName(TEXT("EditStartPoints"));
  if (wb) {
    wb->SetVisible(EnableMultipleStartPoints!=0);
  }

}

#define CHECK_CHANGED(a,b) if (a != b) { changed = true; a = b; }

static void GetWaypointValues(void) {
  WndProperty* wp;
  bool changed = false;

  if (!AATEnabled) {
    return;
  }

  if ((twItemIndex<MAXTASKPOINTS)&&(twItemIndex>=0)) {
    mutexTaskData.Lock();
    wp = (WndProperty*)wf->FindByName(TEXT("prpAATType"));
    if (wp) {
      CHECK_CHANGED(Task[twItemIndex].AATType,
                    wp->GetDataField()->GetAsInteger());
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATCircleRadius"));
    if (wp) {
      CHECK_CHANGED(Task[twItemIndex].AATCircleRadius,
                    iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY));
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATSectorRadius"));
    if (wp) {
      CHECK_CHANGED(Task[twItemIndex].AATSectorRadius,
                    iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY));
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATStartRadial"));
    if (wp) {
      CHECK_CHANGED(Task[twItemIndex].AATStartRadial,
                    wp->GetDataField()->GetAsInteger());
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATFinishRadial"));
    if (wp) {
      CHECK_CHANGED(Task[twItemIndex].AATFinishRadial,
                    wp->GetDataField()->GetAsInteger());
    }
    if (changed) {
      TaskModified = true;
    }
    mutexTaskData.Unlock();

  }
}


static void SetWaypointValues(bool first=false) {
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(TEXT("Cylinder")));
      dfe->addEnumText(gettext(TEXT("Sector")));
    }
    dfe->SetDetachGUI(true); // disable call to OnAATEnabled
    dfe->Set(Task[twItemIndex].AATType);
    dfe->SetDetachGUI(false);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATCircleRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(Task[twItemIndex].AATCircleRadius
                                          *DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->SetVisible(Task[twItemIndex].AATType==0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lround(Task[twItemIndex].AATSectorRadius
                                          *DISTANCEMODIFY*DISTANCE_ROUNDING)/DISTANCE_ROUNDING);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->SetVisible(Task[twItemIndex].AATType>0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATStartRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Task[twItemIndex].AATStartRadial);
    wp->SetVisible(Task[twItemIndex].AATType>0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATFinishRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Task[twItemIndex].AATFinishRadial);
    wp->SetVisible(Task[twItemIndex].AATType>0);
    wp->RefreshDisplay();
  }

}


static void ReadValues(void) {
  WndProperty* wp;
  bool changed = false;

  mutexTaskData.Lock();
  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableMultipleStartPoints"));
  if (wp) {
    CHECK_CHANGED(EnableMultipleStartPoints,
                  wp->GetDataField()->GetAsBoolean());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    CHECK_CHANGED(AATEnabled,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    CHECK_CHANGED(FinishLine,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    CHECK_CHANGED(FinishRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    CHECK_CHANGED(StartLine,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    CHECK_CHANGED(StartRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    CHECK_CHANGED(SectorType,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    CHECK_CHANGED(SectorRadius,
                  (DWORD)iround(wp->GetDataField()->GetAsFloat()
				/DISTANCEMODIFY));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    CHECK_CHANGED(AutoAdvance,
                  wp->GetDataField()->GetAsInteger());
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMinTime"));
  if (wp) {
    CHECK_CHANGED(AATTaskLength,
                  wp->GetDataField()->GetAsInteger());
  }
  if (changed) {
    TaskModified = true;
  }

  mutexTaskData.Unlock();

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
  res = dlgWayPointSelect();
  if (res != -1){
    SelectedWaypoint = res;
    if (Task[twItemIndex].Index != res) {
      if (CheckDeclaration()) {
        mutexTaskData.Lock();
	Task[twItemIndex].Index = res;
        Task[twItemIndex].AATTargetOffsetRadius = 0.0;
        Task[twItemIndex].AATTargetOffsetRadial = 0.0;
        Task[twItemIndex].AATSectorRadius = SectorRadius;
        Task[twItemIndex].AATCircleRadius = SectorRadius;
        Task[twItemIndex].AATTargetLocked = false;
        TaskModified = true;
        mutexTaskData.Unlock();
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
  mutexTaskData.Lock();
  SwapWaypoint(twItemIndex);
  SetWaypointValues();
  mutexTaskData.Unlock();
  wf->SetModalResult(mrOK);
}

static void OnMoveBeforeClicked(WindowControl * Sender){
	(void)Sender;
  mutexTaskData.Lock();
  SwapWaypoint(twItemIndex-1);
  SetWaypointValues();
  mutexTaskData.Unlock();
  wf->SetModalResult(mrOK);
}

static void OnDetailsClicked(WindowControl * Sender){
	(void)Sender;
  SelectedWaypoint = Task[twItemIndex].Index;
  PopupWaypointDetails();
}

static void OnRemoveClicked(WindowControl * Sender) {
	(void)Sender;
  mutexTaskData.Lock();
  RemoveTaskPoint(twItemIndex);
  SetWaypointValues();
  if (ActiveWayPoint>=twItemIndex) {
    ActiveWayPoint--;
  }
  if (ActiveWayPoint<0) {
    ActiveWayPoint= -1;
  }
  mutexTaskData.Unlock();
  wf->SetModalResult(mrOK);
}


static void OnTaskRulesClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetVisible(false);
  if (dlgTaskRules()) {
    TaskModified = true;
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
    if (!ValidTaskPoint(twItemIndex-1)) {
      wb = (WndButton *)wf->FindByName(TEXT("butUp"));
      if (wb) {
        wb->SetVisible(false);
      }
    }
    if (!ValidTaskPoint(twItemIndex+1)) {
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
      if (AATEnabled) {
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
