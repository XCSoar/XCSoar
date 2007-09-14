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
#include "Dialogs.h"
#include "Logger.h"
#include "McReady.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"


static WndForm *wf=NULL;
static WndFrame *wfAdvanced=NULL;
static WndListFrame *wTaskList=NULL;
static WndOwnerDrawFrame *wTaskListEntry = NULL;
static bool showAdvanced= false;

static int UpLimit=0;
static int LowLimit=0;

static int ItemIndex = -1;


static int DrawListIndex=0;

static double lengthtotal = 0.0;
static bool fai_ok = false;

static void OnTaskPaintListItem(WindowControl * Sender, HDC hDC){
  (void)Sender;
  int n = UpLimit - LowLimit;
  TCHAR sTmp[120];
  LockTaskData();

  int p1 = 125;
  int p2 = 175;

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

      ExtTextOut(hDC, p1*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
                 ETO_OPAQUE, NULL,
                 sTmp, _tcslen(sTmp), NULL);
    
      _stprintf(sTmp, TEXT("%d°"),  iround(Task[i].InBound));
      ExtTextOut(hDC, p2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
                 ETO_OPAQUE, NULL,
                 sTmp, _tcslen(sTmp), NULL);
    
    }

  } else {
    if (DrawListIndex==n) {
      _stprintf(sTmp, TEXT("  (%s)"), gettext(TEXT("add waypoint")));
      ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		 ETO_OPAQUE, NULL,
		 sTmp, _tcslen(sTmp), NULL);
    } else if ((DrawListIndex==n+1) && ValidTaskPoint(0)) {

      if (!AATEnabled) {
	_stprintf(sTmp, gettext(TEXT("Total:")));
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
	ExtTextOut(hDC, p1*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   sTmp, _tcslen(sTmp), NULL);

      } else {
	_stprintf(sTmp, TEXT("%s %.0f min"), gettext(TEXT("Total:")),
                  AATTaskLength*1.0);
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
	ExtTextOut(hDC, p1*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   sTmp, _tcslen(sTmp), NULL);
      } 
    }
  }
  UnlockTaskData();

}


static void OverviewRefreshTask(void) {
  LockTaskData();
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
    if ((CALCULATED_INFO.TaskStartTime>0.0)&&(CALCULATED_INFO.Flying)) {
      dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
    }
    wp->GetDataField()->SetAsFloat(dd/60.0);
    wp->RefreshDisplay();
  }
  
  LowLimit =0;
  wTaskList->ResetList();
  wTaskList->Redraw();
  UnlockTaskData();

}



static void UpdateAdvanced(void) {
  if (wfAdvanced) {
    wfAdvanced->SetVisible(showAdvanced);
  }
}


static void OnTaskListEnter(WindowControl * Sender, 
		     WndListFrame::ListInfo_t *ListInfo) {
  (void)Sender;
  ItemIndex = ListInfo->ItemIndex;
  if ((ItemIndex>= UpLimit) || (UpLimit==0)) {
    if (ItemIndex>=UpLimit) {
      ItemIndex= UpLimit;
    }
    // add new waypoint
    if (CheckDeclaration()) {

      if (ItemIndex>0) {
	Task[ItemIndex].Index = Task[0].Index;
      } else {
	if (ValidWayPoint(HomeWaypoint)) {
	  Task[ItemIndex].Index = HomeWaypoint;
	} else {
	  Task[ItemIndex].Index = -1;
	}
      }
      int res;
      res = dlgWayPointSelect();
      if (res != -1){
        Task[ItemIndex].Index = res;
      }
      Task[ItemIndex].AATTargetOffsetRadius = 0.0;
      Task[ItemIndex].AATTargetOffsetRadial = 0.0;
      if (ItemIndex>0) {
	dlgTaskWaypointShowModal(ItemIndex, 2); // finish waypoint
      } else {
	dlgTaskWaypointShowModal(ItemIndex, 0); // start waypoint
      }
      OverviewRefreshTask();
    }
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
	(void)Sender;
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = UpLimit-LowLimit+1;
  } else {
    DrawListIndex = ListInfo->DrawIndex;
    ItemIndex = ListInfo->ItemIndex;
  }
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  ItemIndex = -1; // to stop FormDown bringing up task details
  wf->SetModalResult(mrOK);
}

static void OnClearClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
	(void)ListInfo; (void)Sender;
  if (MessageBoxX(hWndMapWindow,
                  gettext(TEXT("Clear the task?")),
                  gettext(TEXT("Clear task")),
                  MB_YESNO|MB_ICONQUESTION) == IDYES) {
    if (CheckDeclaration()) {
      ClearTask();
      OverviewRefreshTask();
    }
  }
}

static void OnCalcClicked(WindowControl * Sender, 
			  WndListFrame::ListInfo_t *ListInfo){
	(void)Sender;
	(void)ListInfo;
  dlgTaskCalculatorShowModal();
  OverviewRefreshTask();
}


static void OnDeclareClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
	(void)Sender;
	(void)ListInfo;
  RefreshTask();

  LoggerDeviceDeclare();

  // do something here.
}




static void OnSaveClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  (void)ListInfo; (void)Sender;

  int file_index; 
  TCHAR task_name[MAX_PATH];
  TCHAR file_name[MAX_PATH];
  WndProperty* wp;
  DataFieldFileReader *dfe;

  wp = (WndProperty*)wf->FindByName(TEXT("prpFile"));
  if (!wp) return;
  dfe = (DataFieldFileReader*)wp->GetDataField();

  file_index = dfe->GetAsInteger();

  if (file_index==0) {

    // new file... TODO: find a good name not already in the list
    _tcscpy(task_name,TEXT("0"));
    dlgTextEntryShowModal(task_name, 10); // max length

    if (_tcslen(task_name)>0) {

      _tcscat(task_name, TEXT(".tsk"));
      LocalPath(file_name, task_name);

      dfe->Lookup(file_name);
      file_index = dfe->GetAsInteger();

      if (file_index==0) {
        // good, this file is unique..
        dfe->addFile(task_name, file_name);
        dfe->Lookup(file_name);
        wp->RefreshDisplay();
      }

    } else {
      // TODO error, task not saved since no name was given
      return;
    }
  }

  if (file_index>0) {
    // file already exists! ask if want to overwrite

    _stprintf(file_name, TEXT("%s: '%s'"), gettext(TEXT("Task file already exists")),
              dfe->GetAsString());
    if(MessageBoxX(hWndMapWindow,
                   file_name,
                   gettext(TEXT("Overwrite?")),
                   MB_YESNO|MB_ICONQUESTION) != IDYES) {
      return;
    }
  }

  SaveTask(dfe->GetPathFile());
  DoStatusMessage(TEXT("Task saved"));
}


static void OnLoadClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  (void)ListInfo; (void)Sender;

  WndProperty* wp;
  DataFieldFileReader *dfe;

  wp = (WndProperty*)wf->FindByName(TEXT("prpFile"));
  if (!wp) return;
  dfe = (DataFieldFileReader*) wp->GetDataField();
  int file_index = dfe->GetAsInteger();

  if (file_index>0) {
    LoadNewTask(dfe->GetPathFile());
    OverviewRefreshTask();
  } 
}


static void OnAdvancedClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  (void)Sender; (void)ListInfo;
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

  wf = NULL;

  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgTaskOverview_L.xml"));
    wf = dlgLoadFromXML(CallBackTable, 
                        
                        filename, 
                        hWndMainWindow,
                        TEXT("IDR_XML_TASKOVERVIEW_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgTaskOverview.xml"));
    wf = dlgLoadFromXML(CallBackTable, 
                        filename, 
                        hWndMainWindow,
                        TEXT("IDR_XML_TASKOVERVIEW"));
  }

  if (!wf) return;

  ASSERT(wf!=NULL);

  wfAdvanced = ((WndFrame *)wf->FindByName(TEXT("frmAdvanced")));
  ASSERT(wfAdvanced!=NULL);

  wTaskList = (WndListFrame*)wf->FindByName(TEXT("frmTaskList"));
  ASSERT(wTaskList!=NULL);
  wTaskList->SetBorderKind(BORDERLEFT);
  wTaskList->SetEnterCallback(OnTaskListEnter);

  wTaskListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmTaskListEntry"));

  ASSERT(wTaskListEntry!=NULL);
  wTaskListEntry->SetCanFocus(true);

  WndProperty* wp;

  // 

  wp = (WndProperty*)wf->FindByName(TEXT("prpFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.tsk"));
    // JMW TODO on entry to form:       wp->GetDataField()->Lookup(TaskFileName);  (global)
    wp->RefreshDisplay();
  }

  // CALCULATED_INFO.AATTimeToGo
  // 

  // initialise and turn on the display
  OverviewRefreshTask();

  UpdateAdvanced();

  wf->ShowModal();

  // now retrieve back the properties...

  RefreshTask();

  delete wf;

  wf = NULL;

}
