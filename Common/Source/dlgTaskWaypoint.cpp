/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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


#include "stdafx.h"
#include <Aygshell.h>

#include "XCSoar.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "Logger.h"
#include "InfoBoxLayout.h"

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
    dfe->Set(StartLine);
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
    dfe->Set(SectorType);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpOLCEnabled"));
  if (wp) {
    wp->GetDataField()->Set(EnableOLC);
    wp->RefreshDisplay();
  }

  WndButton* wb;
  wb = (WndButton *)wf->FindByName(TEXT("EditStartPoints"));
  if (wb) {
    wb->SetVisible(EnableMultipleStartPoints!=0);
  }

}

static void GetWaypointValues(void) {
  WndProperty* wp;
  if ((twItemIndex<MAXTASKPOINTS)&&(twItemIndex>=0)) {

    wp = (WndProperty*)wf->FindByName(TEXT("prpAATType"));
    if (wp) {
      Task[twItemIndex].AATType = wp->GetDataField()->GetAsInteger();
    }

    int ival;
    
    wp = (WndProperty*)wf->FindByName(TEXT("prpAATCircleRadius"));
    if (wp) {
      ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
      Task[twItemIndex].AATCircleRadius = ival;
    }
    
    wp = (WndProperty*)wf->FindByName(TEXT("prpAATSectorRadius"));
    if (wp) {
      ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
      Task[twItemIndex].AATSectorRadius = ival;
    }
    
    wp = (WndProperty*)wf->FindByName(TEXT("prpAATStartRadial"));
    if (wp) {
      Task[twItemIndex].AATStartRadial = wp->GetDataField()->GetAsInteger();
    }
    
    wp = (WndProperty*)wf->FindByName(TEXT("prpAATFinishRadial"));
    if (wp) {
      Task[twItemIndex].AATFinishRadial = wp->GetDataField()->GetAsInteger();
    }
    
    wp = (WndProperty*)wf->FindByName(TEXT("prpAATOffsetRadius"));
    if (wp) {
      Task[twItemIndex].AATTargetOffsetRadius = 
        wp->GetDataField()->GetAsInteger()/100.0;
    }
  }
}


static void SetWaypointValues(bool first=false) {
  WndProperty* wp;

  // TODO: handle change
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (first) {
      dfe->addEnumText(gettext(TEXT("Cylinder")));
      dfe->addEnumText(gettext(TEXT("Sector")));
    }
    dfe->Set(Task[twItemIndex].AATType);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATOffsetRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Task[twItemIndex].AATTargetOffsetRadius*100.0));
    wp->RefreshDisplay();
  }
}


static void ReadValues(void) {
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableMultipleStartPoints"));
  if (wp) {
    EnableMultipleStartPoints = wp->GetDataField()->GetAsBoolean();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    AATEnabled = wp->GetDataField()->GetAsInteger();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpOLCEnabled"));
  if (wp) {
    EnableOLC = wp->GetDataField()->GetAsBoolean();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    if (FinishLine != wp->GetDataField()->GetAsInteger()) {
      FinishLine = wp->GetDataField()->GetAsInteger();
    }
  }
  
  int ival;

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)FinishRadius != ival) {
      FinishRadius = ival;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    if (StartLine != wp->GetDataField()->GetAsInteger()) {
      StartLine = wp->GetDataField()->GetAsInteger();
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)StartRadius != ival) {
      StartRadius = ival;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    if ((int)SectorType != wp->GetDataField()->GetAsInteger()) {
      SectorType = wp->GetDataField()->GetAsInteger();
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsFloat()/DISTANCEMODIFY);
    if ((int)SectorRadius != ival) {
      SectorRadius = ival;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    if (AutoAdvance != wp->GetDataField()->GetAsInteger()) {
      AutoAdvance = wp->GetDataField()->GetAsInteger();
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMinTime"));
  if (wp) {
    AATTaskLength = wp->GetDataField()->GetAsInteger();
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
  res = dlgWayPointSelect();
  if (res != -1){
    SelectedWaypoint = res;    
    if (Task[twItemIndex].Index != res) {
      if (CheckDeclaration()) {
	Task[twItemIndex].Index = res;
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
  LockTaskData();
  SwapWaypoint(twItemIndex);
  SetWaypointValues();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnMoveBeforeClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  SwapWaypoint(twItemIndex-1);
  SetWaypointValues();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnDetailsClicked(WindowControl * Sender){
	(void)Sender;
  SelectedWaypoint = Task[twItemIndex].Index;
  PopupWaypointDetails();
}

static void OnRemoveClicked(WindowControl * Sender) {
	(void)Sender;
  LockTaskData();
  RemoveTaskPoint(twItemIndex);
  SetWaypointValues();
  if (ActiveWayPoint>=twItemIndex) {
    ActiveWayPoint--;
  }
  if (ActiveWayPoint<0) {
    ActiveWayPoint= -1;
  }
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnSelectClicked),
  DeclearCallBackEntry(OnDetailsClicked),
  DeclearCallBackEntry(OnRemoveClicked),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(OnStartPointClicked),
  DeclearCallBackEntry(OnMoveAfterClicked),
  DeclearCallBackEntry(OnMoveBeforeClicked),
  DeclearCallBackEntry(OnAATEnabled),
  DeclearCallBackEntry(NULL)
};


void dlgTaskWaypointShowModal(int itemindex, int tasktype){
  wf = NULL;
  
  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgTaskWaypoint_L.xml"));
    wf = dlgLoadFromXML(CallBackTable, 
                        
                        filename, 
                        hWndMainWindow,
                        TEXT("IDR_XML_TASKWAYPOINT_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgTaskWaypoint.xml"));
    wf = dlgLoadFromXML(CallBackTable, 
                        filename, 
                        hWndMainWindow,
                        TEXT("IDR_XML_TASKWAYPOINT"));    
  }

  twItemIndex = itemindex;
  twType = tasktype;

  if (!wf) return;

  ASSERT(wf!=NULL);
  //  wf->SetKeyDownNotify(FormKeyDown);

  wStart     = ((WndFrame *)wf->FindByName(TEXT("frmStart")));
  wTurnpoint = ((WndFrame *)wf->FindByName(TEXT("frmTurnpoint")));
  wAATTurnpoint = ((WndFrame *)wf->FindByName(TEXT("frmAATTurnpoint")));
  wFinish    = ((WndFrame *)wf->FindByName(TEXT("frmFinish")));

  ASSERT(wStart!=NULL);
  ASSERT(wTurnpoint!=NULL);
  ASSERT(wAATTurnpoint!=NULL);
  ASSERT(wFinish!=NULL);

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

  // TODO: handle change

  GetWaypointValues();

  ReadValues();

  delete wf;

  wf = NULL;

}
