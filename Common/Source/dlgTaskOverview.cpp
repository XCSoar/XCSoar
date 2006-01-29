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


static WndForm *wf=NULL;
static WndListFrame *wTaskList=NULL;
static WndOwnerDrawFrame *wTaskListEntry = NULL;


static int UpLimit=0;
static int LowLimit=0;

static int ItemIndex = -1;

void dlgTaskWaypointShowModal(int itemindex, int type);


static void UpdateList(void){

  int i;

  //

  LowLimit =0;

  wTaskList->ResetList();
  wTaskList->Redraw();

}



static int DrawListIndex=0;

static double lengthtotal = 0.0;
static bool fai_ok = false;

static void OnTaskPaintListItem(WindowControl * Sender, HDC hDC){

  int n = UpLimit - LowLimit;
  TCHAR sTmp[12];

  if (DrawListIndex < n){
    int i = LowLimit + DrawListIndex;

    if (Task[i].Index>=0) {
      ExtTextOut(hDC, 2, 2,
		 ETO_OPAQUE, NULL,
		 WayPointList[Task[i].Index].Name,
		 _tcslen(WayPointList[Task[i].Index].Name), NULL);
      /*
	if (WayPointList[WayPointSelectInfo[i].Index].Flags & HOME){
      sTmp[0] = 'H';
    }else
    if (WayPointList[WayPointSelectInfo[i].Index].Flags & AIRPORT){
      sTmp[0] = 'A';
    }else
    if (WayPointList[WayPointSelectInfo[i].Index].Flags & LANDPOINT){
      sTmp[0] = 'L';
    }else
      sTmp[0] = 'T';

    ExtTextOut(hDC, 135, 2,
      ETO_OPAQUE, NULL,
      sTmp, 1, NULL);
      */
                           //todo user unit
    _stprintf(sTmp, TEXT("%.0fkm"), Task[i].Leg/1000.0);

    ExtTextOut(hDC, 125, 2,
      ETO_OPAQUE, NULL,
      sTmp, _tcslen(sTmp), NULL);
    
    _stprintf(sTmp, TEXT("%d°"),  iround(Task[i].InBound));
    ExtTextOut(hDC, 175, 2,
      ETO_OPAQUE, NULL,
      sTmp, _tcslen(sTmp), NULL);
    
    }

  } else {
    if (DrawListIndex==n) {
      _stprintf(sTmp, TEXT("%s"), TEXT("..."));
      ExtTextOut(hDC, 2, 2,
		 ETO_OPAQUE, NULL,
		 sTmp, _tcslen(sTmp), NULL);
    } else if (DrawListIndex==n+1) {
      _stprintf(sTmp, TEXT("Total:"));
      ExtTextOut(hDC, 2, 2,
		 ETO_OPAQUE, NULL,
		 sTmp, _tcslen(sTmp), NULL);

      if (fai_ok) {
	_stprintf(sTmp, TEXT("%.0fkm FAI"), lengthtotal/1000.0);
      } else {
	_stprintf(sTmp, TEXT("%.0fkm"), lengthtotal/1000.0);
      }
      ExtTextOut(hDC, 125, 2,
		 ETO_OPAQUE, NULL,
		 sTmp, _tcslen(sTmp), NULL);
    }
  }
}


static void OverviewRefreshTask(void) {
  RefreshTask();

  int i;
  // Only need to refresh info where the removal happened
  // as the order of other taskpoints hasn't changed
  UpLimit = 0;
  lengthtotal = 0;
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (Task[i].Index != -1) {
      lengthtotal += Task[i].Leg;
      UpLimit = i;
    }
  }
  UpLimit++;

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

  UpdateList();

  /*
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(lengthtotal/1000.0);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFAIOK"));
  if (wp) {
    wp->GetDataField()->Set(fai_ok);
    wp->RefreshDisplay();
  }
  */

}


static void ReadValues(void) {
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpMinTime"));
  if (wp) {
    AATTaskLength = wp->GetDataField()->GetAsInteger();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    AATEnabled = wp->GetDataField()->GetAsInteger();
  }

}

static void OnTaskListEnter(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo) {

  ReadValues();

  ItemIndex = ListInfo->ItemIndex;
  if ((ItemIndex>= UpLimit) || (UpLimit==1)) {
    ItemIndex= UpLimit;
    Task[ItemIndex].Index = Task[0].Index;
    dlgTaskWaypointShowModal(ItemIndex, 2); // finish waypoint
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

static void OnDeclareClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  ReadValues();
  RefreshTask();
}

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnTaskPaintListItem),
  DeclearCallBackEntry(OnTaskListInfo),
  DeclearCallBackEntry(OnDeclareClicked),
  DeclearCallBackEntry(OnClearClicked),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};



void dlgTaskOverviewShowModal(void){

  UpLimit = 0;
  LowLimit = 0;
  ItemIndex = -1;

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgTaskOverview.xml", 
		      hWndMainWindow);

  if (!wf) return;

  ASSERT(wf!=NULL);

  wTaskList = (WndListFrame*)wf->FindByName(TEXT("frmTaskList"));
  ASSERT(wTaskList!=NULL);
  wTaskList->SetBorderKind(BORDERLEFT);
  wTaskList->SetEnterCallback(OnTaskListEnter);

  wTaskListEntry = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmTaskListEntry"));
  ASSERT(wTaskListEntry!=NULL);
  wTaskListEntry->SetCanFocus(true);

  WndProperty* wp;

  // set properties...

  wp = (WndProperty*)wf->FindByName(TEXT("prpMinTime"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(AATTaskLength);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEnabled"));
  if (wp) {
    bool aw = AATEnabled;
    wp->GetDataField()->Set(aw);
    wp->RefreshDisplay();
  }

  // initialise and turn on the display
  OverviewRefreshTask();
  wf->ShowModal();

  // now retrieve back the properties...

  ReadValues();
  RefreshTask();

  delete wf;

  wf = NULL;

}

