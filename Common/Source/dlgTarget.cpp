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

#include "statistics.h"

#include "externs.h"
#include "units.h"
#include "McReady.h"
#include "device.h"

#include "WindowControls.h"
#include "dlgTools.h"
#include "Port.h"
#include "Calculations2.h"
#include "MapWindow.h"
#include "InfoBoxLayout.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;


static void OnOKClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static double Range = 0;
static double Radial = 0;
static int target_point = 0;


static void RefreshCalculator(void) {
  WndProperty* wp;

  RefreshTask();
  RefreshTaskStatistics();

  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    wp->RefreshDisplay();
    if (!AATEnabled || (target_point==0) || !ValidTaskPoint(target_point+1)) {
      wp->SetVisible(false);
    } else {
      wp->SetVisible(true);
    }
    wp->GetDataField()->SetAsFloat(Range*100.0);
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRadial"));
  if (wp) {
    wp->RefreshDisplay();
    if (!AATEnabled || (target_point==0) || !ValidTaskPoint(target_point+1)) {
      wp->SetVisible(false);
    } else {
      wp->SetVisible(true);
    }
    wp->GetDataField()->SetAsFloat(Radial);
  }

  // update outputs
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEst"));
  if (wp) {
    double dd = CALCULATED_INFO.TaskTimeToGo;
    if ((CALCULATED_INFO.TaskStartTime>0.0)&&(CALCULATED_INFO.Flying)) {
      dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
    }
    dd= min(24.0*60.0,dd/60.0);
    wp->GetDataField()->SetAsFloat(dd);
    wp->RefreshDisplay();
  }

  double v1;
  if (CALCULATED_INFO.TaskTimeToGo>0) {
    v1 = CALCULATED_INFO.TaskDistanceToGo/
      CALCULATED_INFO.TaskTimeToGo;
  } else {
    v1 = 0;
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeedRemaining"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(v1*TASKSPEEDMODIFY);
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeedAchieved"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(CALCULATED_INFO.TaskSpeed*TASKSPEEDMODIFY);
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

}


static void OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode) {
  switch(Mode){
    case DataField::daGet:
      //      Sender->Set(Range*100.0);
    break;
    case DataField::daPut:
    case DataField::daChange:
      Range = Sender->GetAsFloat()/100.0;
      LockTaskData();
      Task[target_point].AATTargetOffsetRadius = Range;
      UnlockTaskData();
      RefreshCalculator();
    break;
  }
}


static void OnRadialData(DataField *Sender, DataField::DataAccessKind_t Mode) {
  switch(Mode){
    case DataField::daGet:
      //      Sender->Set(Range*100.0);
    break;
    case DataField::daPut:
    case DataField::daChange:
      Radial = Sender->GetAsFloat();
      LockTaskData();
      Task[target_point].AATTargetOffsetRadial = Radial;
      UnlockTaskData();
      RefreshCalculator();
    break;
  }
}


static void RefreshTargetPoint(void) {
  LockTaskData();
  if (ValidTaskPoint(target_point)) {
    MapWindow::SetTargetPan(true, target_point);
    Range = Task[target_point].AATTargetOffsetRadius;
    Radial = Task[target_point].AATTargetOffsetRadial;
  } else {
    Range = 0;
    Radial = 0;
  }
  UnlockTaskData();
  RefreshCalculator();
}


static void OnTaskPointData(DataField *Sender, DataField::DataAccessKind_t Mode) {
  switch(Mode){
    case DataField::daGet:
      //      Sender->Set(Range*100.0);
    break;
    case DataField::daPut:
    case DataField::daChange:
      target_point = Sender->GetAsInteger() + ActiveWayPoint;
      RefreshTargetPoint();
    break;
  }
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnTaskPointData),
  DeclearCallBackEntry(OnRangeData),
  DeclearCallBackEntry(OnRadialData),
  DeclearCallBackEntry(OnOKClicked),
  DeclearCallBackEntry(NULL)
};


void dlgTarget(void) {

  if (!ValidTaskPoint(ActiveWayPoint)) {
    return;
  }

  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgTarget_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_TARGET_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgTarget.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_TARGET"));
  }

  if (!wf) return;

  /*
  if (!AATEnabled) {
    ((WndButton *)wf->FindByName(TEXT("Optimise")))->SetVisible(false);
  }
  */

  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskPoint"));
  DataFieldEnum* dfe;
  dfe = (DataFieldEnum*)wp->GetDataField();
  TCHAR tp_label[80];
  TCHAR tp_short[21];
  LockTaskData();
  if (!ValidTaskPoint(target_point)) {
    target_point = ActiveWayPoint;
  } else {
    target_point = max(target_point, ActiveWayPoint);
  }
  for (int i=ActiveWayPoint; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(i)) {
      _tcsncpy(tp_short, WayPointList[Task[i].Index].Name, 20);
      tp_short[20] = 0;
      _stprintf(tp_label, TEXT("%d %s"), i, tp_short);
      dfe->addEnumText(tp_label);
    } else {
      if (target_point>= i) {
        target_point= ActiveWayPoint;
      }
    }
  }
  dfe->Set(target_point-ActiveWayPoint);
  UnlockTaskData();
  wp->RefreshDisplay();

  RefreshTargetPoint();

  wf->ShowModal();

  MapWindow::SetTargetPan(false, 0);

  delete wf;
  wf = NULL;
}
