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
#if (NEWINFOBOX>0)


#include "stdafx.h"
#include <Aygshell.h>

#include "XCSoar.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "Logger.h"

static int twItemIndex= 0;
static WndForm *wf=NULL;
static int twType = 0; // start, turnpoint, finish

static WndFrame *wStart=NULL;
static WndFrame *wTurnpoint=NULL;
static WndFrame *wAATTurnpoint=NULL;
static WndFrame *wFinish=NULL;

int dlgWayPointSelect(void);

static void UpdateCaption(void) {
  TCHAR sTmp[128];
  if (Task[twItemIndex].Index != -1) {
    switch (twType) {
    case 0:
      _stprintf(sTmp, TEXT("Start: %s"), 
		WayPointList[Task[twItemIndex].Index].Name);
      break;
    case 1:
      _stprintf(sTmp, TEXT("Turnpoint: %s"), 
		WayPointList[Task[twItemIndex].Index].Name);
      break;
    case 2:
      _stprintf(sTmp, TEXT("Finish: %s"), 
		WayPointList[Task[twItemIndex].Index].Name);
      break;
    };
    wf->SetCaption(sTmp);
  } else {
    wf->SetCaption(TEXT("(invalid)"));
  }
}


static void SetValues(void) {
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("Line"));
    dfe->Set(FinishLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(FinishRadius*2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("Line"));
    dfe->Set(StartLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(StartRadius*2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("FAI Sector"));
    dfe->addEnumText(TEXT("DAe 0.5/10"));
    dfe->Set(SectorType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(SectorRadius);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Manual"));
    dfe->addEnumText(TEXT("Auto"));
    dfe->addEnumText(TEXT("Arm"));
    dfe->addEnumText(TEXT("Arm start"));
    dfe->Set(AutoAdvance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMinTime"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(AATTaskLength);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    bool aw = (AATEnabled != 0);
    wp->GetDataField()->Set(aw);
    wp->RefreshDisplay();
  }
}


static void ReadValues(void) {
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    AATEnabled = wp->GetDataField()->GetAsInteger();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    if (FinishLine != wp->GetDataField()->GetAsInteger()) {
      FinishLine = wp->GetDataField()->GetAsInteger();
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    if ((int)FinishRadius*2 != wp->GetDataField()->GetAsInteger()) {
      FinishRadius = wp->GetDataField()->GetAsInteger()/2;
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
    if ((int)StartRadius*2 != wp->GetDataField()->GetAsInteger()) {
      StartRadius = wp->GetDataField()->GetAsInteger()/2;
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
    if ((int)SectorRadius != wp->GetDataField()->GetAsInteger()) {
      SectorRadius = wp->GetDataField()->GetAsInteger();
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


static void OnSelectClicked(WindowControl * Sender){
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
  wf->SetModalResult(mrOK);
}


static void OnDetailsClicked(WindowControl * Sender){
  SelectedWaypoint = Task[twItemIndex].Index;
  PopupWaypointDetails();
}

static void OnRemoveClicked(WindowControl * Sender) {
  LockFlightData();
  RemoveTaskPoint(twItemIndex);
  if (ActiveWayPoint>=twItemIndex) {
    ActiveWayPoint--;
  }
  if (ActiveWayPoint<0) {
    ActiveWayPoint= -1;
  }
  UnlockFlightData();
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnSelectClicked),
  DeclearCallBackEntry(OnDetailsClicked),
  DeclearCallBackEntry(OnRemoveClicked),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};


void dlgTaskWaypointShowModal(int itemindex, int tasktype){

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgTaskWaypoint.xml", 
		      hWndMainWindow);

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

  WndProperty* wp;

  // TODO: handle change
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("Sector"));
    dfe->Set(Task[twItemIndex].AATType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATCircleRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Task[twItemIndex].AATCircleRadius);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Task[twItemIndex].AATSectorRadius);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATStartRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Task[twItemIndex].AATStartRadial);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATFinishRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Task[twItemIndex].AATFinishRadial);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATOffsetRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(Task[twItemIndex].AATTargetOffsetRadius*100.0));
    wp->RefreshDisplay();
  }

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

  SetValues();

  UpdateCaption();

  wf->ShowModal();

  // now retrieve changes

  // TODO: handle change
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATType"));
  if (wp) {
    Task[twItemIndex].AATType = wp->GetDataField()->GetAsInteger();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATCircleRadius"));
  if (wp) {
    Task[twItemIndex].AATCircleRadius = wp->GetDataField()->GetAsInteger();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATSectorRadius"));
  if (wp) {
    Task[twItemIndex].AATSectorRadius = wp->GetDataField()->GetAsInteger();
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

  ReadValues();

  delete wf;

  wf = NULL;

}

#endif
