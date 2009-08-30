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
#include "Calculations.h" // for RefreshTaskStatistics()
#include "Blackboard.hpp"
#include "SettingsTask.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Logger.h"
#include "McReady.h"
#include "Dialogs/dlgTools.h"
#include "InfoBoxLayout.h"
#include "Math/FastMath.h"
#include "Screen/Util.hpp"
#include "Screen/MainWindow.hpp"
#include "LocalPath.hpp"
#include "DataField/FileReader.hpp"

#include <assert.h>

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

static void UpdateFilePointer(void) {
  WndProperty *wp = (WndProperty*)wf->FindByName(TEXT("prpFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    if (_tcslen(LastTaskFileName)>0) {
      dfe->Lookup(LastTaskFileName);
    } else {
      dfe->Set(0);
    }
    wp->RefreshDisplay();
  }
}


static void UpdateCaption (void) {
  TCHAR title[MAX_PATH];
  TCHAR name[MAX_PATH] = TEXT("\0");
  mutexTaskData.Lock();
  int len = _tcslen(LastTaskFileName);
  if (len>0) {
    int index = 0;
    TCHAR *src = LastTaskFileName;
    while ((*src != _T('\0')) && (*src != _T('.'))) {
      if ((*src == _T('\\')) || (*src == _T('/'))) {
        index = 0;
      } else {
        name[index] = *src;
        index++;
      }
      src++;
    }
    name[index]= _T('\0');
  }
  mutexTaskData.Unlock();

  if (_tcslen(name)>0) {
    _stprintf(title, TEXT("%s: %s"),
              gettext(TEXT("Task Overview")),
              name);
  } else {
    _stprintf(title, TEXT("%s"),
              gettext(TEXT("Task Overview")));
  }

  if (TaskModified) {
    _tcscat(title, TEXT(" *"));
  }

  wf->SetCaption(title);
}


static void OnTaskPaintListItem(WindowControl * Sender, HDC hDC){
  (void)Sender;
  int n = UpLimit - LowLimit;
  TCHAR sTmp[120];
  mutexTaskData.Lock();

  int w0;
  if (InfoBoxLayout::landscape) {
    w0 = 200*InfoBoxLayout::scale;
  } else {
    w0 = 210*InfoBoxLayout::scale;
  }
  int w1 = GetTextWidth(hDC, TEXT(" 000km"));
  int w2 = GetTextWidth(hDC, TEXT("  000")TEXT(DEG));

  int p1 = w0-w1-w2; // 125*InfoBoxLayout::scale;
  int p2 = w0-w2;    // 175*InfoBoxLayout::scale;

  if (DrawListIndex < n){
    int i = LowLimit + DrawListIndex;

    if (Task[i].Index>=0) {
      if (InfoBoxLayout::landscape &&
          AATEnabled && ValidTaskPoint(i+1) && (i>0)) {
        if (Task[i].AATType==0) {
          _stprintf(sTmp, TEXT("%s %.1f"),
                    WayPointList[Task[i].Index].Name,
                    Task[i].AATCircleRadius*DISTANCEMODIFY);
        } else {
          _stprintf(sTmp, TEXT("%s %.1f"),
                    WayPointList[Task[i].Index].Name,
                    Task[i].AATSectorRadius*DISTANCEMODIFY);
        }
      } else {
        _stprintf(sTmp, TEXT("%s"),
                  WayPointList[Task[i].Index].Name);
      }

      ExtTextOutClip(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		     sTmp, p1-4*InfoBoxLayout::scale);

      _stprintf(sTmp, TEXT("%.0f %s"),
		Task[i].Leg*DISTANCEMODIFY,
		Units::GetDistanceName());
      ExtTextOut(hDC, p1+w1-GetTextWidth(hDC, sTmp),
                 2*InfoBoxLayout::scale,
                 ETO_OPAQUE, NULL,
                 sTmp, _tcslen(sTmp), NULL);

      _stprintf(sTmp, TEXT("%d")TEXT(DEG),  iround(Task[i].InBound));
      ExtTextOut(hDC, p2+w2-GetTextWidth(hDC, sTmp),
                 2*InfoBoxLayout::scale,
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
	ExtTextOut(hDC, p1+w1-GetTextWidth(hDC, sTmp),
                   2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   sTmp, _tcslen(sTmp), NULL);

      } else {

	double d1 = (CALCULATED_INFO.TaskDistanceToGo
		     +CALCULATED_INFO.TaskDistanceCovered);
	if (d1==0.0) {
	  d1 = CALCULATED_INFO.AATTargetDistance;
	}

	_stprintf(sTmp, TEXT("%s %.0f min %.0f (%.0f) %s"),
                  gettext(TEXT("Total:")),
                  AATTaskLength*1.0,
		  DISTANCEMODIFY*lengthtotal,
		  DISTANCEMODIFY*d1,
		  Units::GetDistanceName());
	ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   sTmp, _tcslen(sTmp), NULL);
      }
    }
  }
  mutexTaskData.Unlock();

}


static void OverviewRefreshTask(void) {
  mutexTaskData.Lock();
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

  UpdateCaption();
  mutexTaskData.Unlock();

}



static void UpdateAdvanced(void) {
  if (wfAdvanced) {
    wfAdvanced->SetVisible(showAdvanced);
  }
}


static void OnTaskListEnter(WindowControl * Sender,
		     WndListFrame::ListInfo_t *ListInfo) {
  (void)Sender;
  bool isfinish = false;
  ItemIndex = ListInfo->ItemIndex;
  if ((ItemIndex>= UpLimit) || (UpLimit==0)) {
    if (ItemIndex>=UpLimit) {
      ItemIndex= UpLimit;
    }
    // add new waypoint
    if (CheckDeclaration()) {

      if (ItemIndex>0) {
        if (MessageBoxX(gettext(TEXT("Will this be the finish?")),
                        gettext(TEXT("Add Waypoint")),
                        MB_YESNO|MB_ICONQUESTION) == IDYES) {
          isfinish = true;
        } else {
          isfinish = false;
        }
      }

      mutexTaskData.Lock();
      if (ItemIndex>0) {
	Task[ItemIndex].Index = Task[0].Index;
      } else {
	if (ValidWayPoint(HomeWaypoint)) {
	  Task[ItemIndex].Index = HomeWaypoint;
	} else {
	  Task[ItemIndex].Index = -1;
	}
      }
      mutexTaskData.Unlock();

      int res;
      res = dlgWayPointSelect();
      mutexTaskData.Lock();
      if (res != -1){
        Task[ItemIndex].Index = res;
      }
      Task[ItemIndex].AATTargetOffsetRadius = 0.0;
      Task[ItemIndex].AATTargetOffsetRadial = 0.0;
      Task[ItemIndex].AATSectorRadius = SectorRadius;
      Task[ItemIndex].AATCircleRadius = SectorRadius;
      Task[ItemIndex].AATTargetLocked = false;
      mutexTaskData.Unlock();

      if (ItemIndex==0) {
	dlgTaskWaypointShowModal(ItemIndex, 0, true); // start waypoint
      } else if (isfinish) {
        dlgTaskWaypointShowModal(ItemIndex, 2, true); // finish waypoint
      } else {
        if (AATEnabled) {
          // only need to set properties for finish
          dlgTaskWaypointShowModal(ItemIndex, 1, true); // normal waypoint
        }
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
  if (MessageBoxX(gettext(TEXT("Clear the task?")),
                  gettext(TEXT("Clear task")),
                  MB_YESNO|MB_ICONQUESTION) == IDYES) {
    if (CheckDeclaration()) {
      ClearTask();
      UpdateFilePointer();
      OverviewRefreshTask();
      UpdateCaption();
    }
  }
}

static void OnCalcClicked(WindowControl * Sender,
			  WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
  (void)ListInfo;

  wf->SetVisible(false);
  dlgTaskCalculatorShowModal();
  OverviewRefreshTask();
  wf->SetVisible(true);
}


static void OnAnalysisClicked(WindowControl * Sender,
                              WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
  (void)ListInfo;

  wf->SetVisible(false);
  dlgAnalysisShowModal();
  wf->SetVisible(true);
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

    // TODO enhancement: suggest a good new name not already in the list
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
      // TODO code: report error, task not saved since no name was given
      return;
    }
  }

  if (file_index>0) {
    // file already exists! ask if want to overwrite

    _stprintf(file_name, TEXT("%s: '%s'"),
              gettext(TEXT("Task file already exists")),
              dfe->GetAsString());
    if(MessageBoxX(file_name,
                   gettext(TEXT("Overwrite?")),
                   MB_YESNO|MB_ICONQUESTION) != IDYES) {
      return;
    }
  }

  SaveTask(dfe->GetPathFile());
  UpdateCaption();
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
    UpdateFilePointer();
    UpdateCaption();
  }
}


static void OnAdvancedClicked(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  (void)Sender; (void)ListInfo;
  showAdvanced = !showAdvanced;
  UpdateAdvanced();
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnTaskPaintListItem),
  DeclareCallBackEntry(OnTaskListInfo),
  DeclareCallBackEntry(OnDeclareClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(OnClearClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnAdvancedClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnLoadClicked),
  DeclareCallBackEntry(OnAnalysisClicked),
  DeclareCallBackEntry(NULL)
};


void dlgTaskOverviewShowModal(void){

  UpLimit = 0;
  LowLimit = 0;
  ItemIndex = -1;

  showAdvanced = false;

  wf = NULL;

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgTaskOverview_L.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_TASKOVERVIEW_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgTaskOverview.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_TASKOVERVIEW"));
  }

  if (!wf) return;

  assert(wf!=NULL);

  UpdateCaption();

  wfAdvanced = ((WndFrame *)wf->FindByName(TEXT("frmAdvanced")));
  assert(wfAdvanced!=NULL);

  wTaskList = (WndListFrame*)wf->FindByName(TEXT("frmTaskList"));
  assert(wTaskList!=NULL);
  wTaskList->SetBorderKind(BORDERLEFT);
  wTaskList->SetEnterCallback(OnTaskListEnter);

  wTaskListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmTaskListEntry"));

  assert(wTaskListEntry!=NULL);
  wTaskListEntry->SetCanFocus(true);

  WndProperty* wp;

  //

  wp = (WndProperty*)wf->FindByName(TEXT("prpFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.tsk"));
    wp->RefreshDisplay();
  }
  UpdateFilePointer();

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
