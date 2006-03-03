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
#include "Dialogs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"


void dlgTaskCalculatorShowModal(void);

static WndForm *wf=NULL;
static WndFrame *wfAdvanced=NULL;
static WndListFrame *wTaskList=NULL;
static WndOwnerDrawFrame *wTaskListEntry = NULL;
static bool showAdvanced= false;

static int UpLimit=0;
static int LowLimit=0;

static int ItemIndex = -1;

void dlgTaskWaypointShowModal(int itemindex, int type);


static int DrawListIndex=0;

static double lengthtotal = 0.0;
static bool fai_ok = false;

static void OnTaskPaintListItem(WindowControl * Sender, HDC hDC){

  int n = UpLimit - LowLimit;
  TCHAR sTmp[120];
  LockFlightData();

  if (DrawListIndex < n){
    int i = LowLimit + DrawListIndex;

    if (Task[i].Index>=0) {
      ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		 ETO_OPAQUE, NULL,
		 WayPointList[Task[i].Index].Name,
		 _tcslen(WayPointList[Task[i].Index].Name), NULL);

      _stprintf(sTmp, TEXT("%.0f %s"),
		Task[i].Leg*DISTANCEMODIFY,
		Units::GetDistanceName());

      ExtTextOut(hDC, 125*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
      ETO_OPAQUE, NULL,
      sTmp, _tcslen(sTmp), NULL);

    _stprintf(sTmp, TEXT("%d°"),  iround(Task[i].InBound));
    ExtTextOut(hDC, 175*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
      ETO_OPAQUE, NULL,
      sTmp, _tcslen(sTmp), NULL);

    }

  } else {
    if (DrawListIndex==n) {
      _stprintf(sTmp, TEXT("%s"), TEXT("..."));
      ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		 ETO_OPAQUE, NULL,
		 sTmp, _tcslen(sTmp), NULL);
    } else if (DrawListIndex==n+1) {

      if (!AATEnabled) {
	_stprintf(sTmp, TEXT("Total:"));
	ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   sTmp, _tcslen(sTmp), NULL);

	if (fai_ok) {
	  _stprintf(sTmp, TEXT("%.0f %s FAI"), lengthtotal*DISTANCEMODIFY,
		    Units::GetDistanceName());
	} else {
	  _stprintf(sTmp, TEXT("%.0f %s"), lengthtotal*DISTANCEMODIFY,
		    Units::GetDistanceName());
	}
	ExtTextOut(hDC, 125*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   sTmp, _tcslen(sTmp), NULL);

      } else {
	_stprintf(sTmp, TEXT("Total: %.0f min"), AATTaskLength*1.0);
	ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   sTmp, _tcslen(sTmp), NULL);

	double d1 = (CALCULATED_INFO.TaskDistanceToGo
		     +CALCULATED_INFO.TaskDistanceCovered);
	if (d1==0.0) {
	  d1 = CALCULATED_INFO.AATTargetDistance;
	}
	_stprintf(sTmp, TEXT("%.0f (%.0f) %s"),
		  DISTANCEMODIFY*lengthtotal,
		  DISTANCEMODIFY*d1,
		  Units::GetDistanceName());
	ExtTextOut(hDC, 125*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   sTmp, _tcslen(sTmp), NULL);
      }
    }
  }
  UnlockFlightData();

}


static void OverviewRefreshTask(void) {
  LockFlightData();
  RefreshTask();

  int i;
  // Only need to refresh info where the removal happened
  // as the order of other taskpoints hasn't changed
  UpLimit = 0;
  lengthtotal = 0;
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (Task[i].Index != -1) {
      lengthtotal += Task[i].Leg;
      UpLimit = i+1;
    }
  }

  // Simple FAI 2004 triangle rules
  fai_ok = true;
  if (lengthtotal>0) {
    for (i=0; i<MAXTASKPOINTS; i++) {
      if (Task[i].Index != -1) {
	double lrat = Task[i].Leg/lengthtotal;
	if ((lrat>0.45)||(lrat<0.10)) {
	  fai_ok = false;
	}
      }
    }
  } else {
    fai_ok = false;
  }

  RefreshTaskStatistics();

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEst"));
  if (wp) {
    double dd = CALCULATED_INFO.TaskTimeToGo;
    if (CALCULATED_INFO.TaskStartTime>0.0) {
      dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
    }
    wp->GetDataField()->SetAsFloat(dd/60.0);
    wp->RefreshDisplay();
  }

  LowLimit =0;
  wTaskList->ResetList();
  wTaskList->Redraw();
  UnlockFlightData();

}



static void UpdateAdvanced(void) {
  if (wfAdvanced) {
    wfAdvanced->SetVisible(showAdvanced);
  }
}


static void OnTaskListEnter(WindowControl * Sender,
		     WndListFrame::ListInfo_t *ListInfo) {

  ItemIndex = ListInfo->ItemIndex;
  if ((ItemIndex>= UpLimit) || (UpLimit==1)) {
    if (ItemIndex>=UpLimit) {
      ItemIndex= UpLimit;
    }
    // create new waypoint
    if (ItemIndex>0) {
      Task[ItemIndex].Index = Task[0].Index;
    } else {
      if (HomeWaypoint>=0) {
	Task[ItemIndex].Index = HomeWaypoint;
      } else {
	Task[ItemIndex].Index = -1;
      }
    }
    Task[ItemIndex].AATTargetOffsetRadius = 0.0;
    Task[ItemIndex].AATTargetOffsetRadial = 0.0;
    if (ItemIndex>0) {
      dlgTaskWaypointShowModal(ItemIndex, 2); // finish waypoint
    } else {
      dlgTaskWaypointShowModal(ItemIndex, 0); // start waypoint
    }
    OverviewRefreshTask();
    return;
  }
  if (ItemIndex==0) {
    dlgTaskWaypointShowModal(ItemIndex, 0); // start waypoint
  } else if (ItemIndex==UpLimit-1) {
    dlgTaskWaypointShowModal(ItemIndex, 2); // finish waypoint
  } else {
    dlgTaskWaypointShowModal(ItemIndex, 1); // turnpoint
  }
  OverviewRefreshTask();

}


static void OnTaskListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = UpLimit-LowLimit+1;
  } else {
    DrawListIndex = ListInfo->DrawIndex;
    ItemIndex = ListInfo->ItemIndex;
  }
}

static void OnCloseClicked(WindowControl * Sender){
  ItemIndex = -1; // to stop FormDown bringing up task details
  wf->SetModalResult(mrOK);
}

static void OnClearClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){

}

static void OnCalcClicked(WindowControl * Sender,
			  WndListFrame::ListInfo_t *ListInfo){
  dlgTaskCalculatorShowModal();
  OverviewRefreshTask();
}


static void OnDeclareClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  RefreshTask();
  // do something here.
}

static int TaskFileNumber = 0;

static void GetTaskFileName(TCHAR *filename) {
  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpFile"));
  if (wp) {
    TaskFileNumber = wp->GetDataField()->GetAsInteger();
  }
#if (WINDOWSPC>0)
  _stprintf(filename,TEXT("C:\\XCSoar\\NOR Flash\\%02d.tsk"),
	    TaskFileNumber);
#ifdef GNAV
  _stprintf(filename,TEXT("\\NOR Flash\\%02d.tsk"),
	    TaskFileNumber);
#else
  _stprintf(filename,TEXT("\\%02d.tsk"),
	    TaskFileNumber);
#endif
#endif
}

static void OnSaveClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  TCHAR filename[100];
  GetTaskFileName(filename);
  SaveTask(filename);
  DoStatusMessage(TEXT("Task saved"));
}


static void OnLoadClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  TCHAR filename[100];
  GetTaskFileName(filename);
  LoadNewTask(filename);
  OverviewRefreshTask();
}

static void OnAdvancedClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  showAdvanced = !showAdvanced;
  UpdateAdvanced();
}

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnTaskPaintListItem),
  DeclearCallBackEntry(OnTaskListInfo),
  DeclearCallBackEntry(OnDeclareClicked),
  DeclearCallBackEntry(OnCalcClicked),
  DeclearCallBackEntry(OnClearClicked),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(OnAdvancedClicked),
  DeclearCallBackEntry(OnSaveClicked),
  DeclearCallBackEntry(OnLoadClicked),
  DeclearCallBackEntry(NULL)
};



void dlgTaskOverviewShowModal(void){

  UpLimit = 0;
  LowLimit = 0;
  ItemIndex = -1;

  showAdvanced = false;

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgTaskOverview.xml",
		      hWndMainWindow);

  if (!wf) return;

  ASSERT(wf!=NULL);

  wfAdvanced = ((WndFrame *)wf->FindByName(TEXT("frmAdvanced")));
  ASSERT(wfAdvanced!=NULL);

  wTaskList = (WndListFrame*)wf->FindByName(TEXT("frmTaskList"));
  ASSERT(wTaskList!=NULL);
  wTaskList->SetBorderKind(BORDERLEFT);
  wTaskList->SetEnterCallback(OnTaskListEnter);

  wTaskListEntry = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmTaskListEntry"));
  ASSERT(wTaskListEntry!=NULL);
  wTaskListEntry->SetCanFocus(true);

  WndProperty* wp;

  //

  wp = (WndProperty*)wf->FindByName(TEXT("prpFile"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(TaskFileNumber);
    wp->RefreshDisplay();
  }

  // CALCULATED_INFO.AATTimeToGo
  //

  // initialise and turn on the display
  OverviewRefreshTask();

  /*
  WndButton *wb;

  wb = (WndButton*)wf->FindByName(TEXT("cmdClose"));
  if (wb) {
    SetFocus(wb->GetHandle());
  }
  */

  UpdateAdvanced();

  wf->ShowModal();

  // now retrieve back the properties...

  RefreshTask();

  delete wf;

  wf = NULL;

}

#endif
