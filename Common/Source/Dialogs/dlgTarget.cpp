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

#include "Dialogs.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Interface.hpp"
#include <math.h>
#include "Blackboard.hpp"
#include "SettingsTask.hpp"
#include "Units.hpp"
#include "Math/Earth.hpp"
#include "LogFile.hpp"
#include "Dialogs/dlgTools.h"
#include "Calculations.h"
#include "MapWindow.h"
#include "InfoBoxLayout.h"
#include "Math/Geometry.hpp"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "WayPoint.hpp"
#include "Protection.hpp"

static WndForm *wf=NULL;
static WindowControl *btnMove = NULL;
static int ActiveWayPointOnEntry = 0;


static double Range = 0;
static double Radial = 0;
static int target_point = 0;
static bool TargetMoveMode = false;

static void OnOKClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetModalResult(mrOK);
}




static void MoveTarget(double adjust_angle) {
  if (!AATEnabled) return;
  if (target_point==0) return;
  if (!ValidTaskPoint(target_point)) return;
  if (!ValidTaskPoint(target_point+1)) return;
  if (target_point < ActiveWayPoint) return;

  mutexTaskData.Lock();

  double target_latitude, target_longitude;
  double bearing, distance;
  distance = 500;
  if(task_points[target_point].AATType == SECTOR) {
    distance = max(task_points[target_point].AATSectorRadius/20.0,distance);
  } else {
    distance = max(task_points[target_point].AATCircleRadius/20.0,distance);
  }

  bearing = AngleLimit360(XCSoarInterface::main_window.map.GetDisplayAngle() + adjust_angle);
  FindLatitudeLongitude (task_points[target_point].AATTargetLat,
                         task_points[target_point].AATTargetLon,
                         bearing,
                         distance,
                         &target_latitude,
                         &target_longitude);

  if (InAATTurnSector(target_longitude, target_latitude, target_point)) {
    if (XCSoarInterface::Calculated().IsInSector && (target_point == ActiveWayPoint)) {
      // set range/radial for inside sector
      double course_bearing, target_bearing;
      DistanceBearing(task_points[target_point-1].AATTargetLat,
                      task_points[target_point-1].AATTargetLon,
                      XCSoarInterface::Basic().Latitude,
                      XCSoarInterface::Basic().Longitude,
                      NULL, &course_bearing);

      DistanceBearing(XCSoarInterface::Basic().Latitude,
                      XCSoarInterface::Basic().Longitude,
                      target_latitude,
                      target_longitude,
                      &distance, &target_bearing);
      bearing = AngleLimit180(target_bearing-course_bearing);

      if (fabs(bearing)<90.0) {
        task_points[target_point].AATTargetLat = target_latitude;
        task_points[target_point].AATTargetLon = target_longitude;
        Radial = bearing;
        task_points[target_point].AATTargetOffsetRadial = Radial;
        Range =
          FindInsideAATSectorRange(XCSoarInterface::Basic().Latitude,
                                   XCSoarInterface::Basic().Longitude,
                                   target_point,
                                   target_bearing,
                                   distance);
        task_points[target_point].AATTargetOffsetRadius = Range;
        SetTargetModified();
      }
    } else {
      // OK to change it..
      task_points[target_point].AATTargetLat = target_latitude;
      task_points[target_point].AATTargetLon = target_longitude;

      // set range/radial for outside sector
      DistanceBearing(WayPointList[task_points[target_point].Index].Latitude,
                      WayPointList[task_points[target_point].Index].Longitude,
                      task_points[target_point].AATTargetLat,
                      task_points[target_point].AATTargetLon,
                      &distance, &bearing);
      bearing = AngleLimit180(bearing-task_points[target_point].Bisector);
      if(task_points[target_point].AATType == SECTOR) {
        Range = (fabs(distance)/task_points[target_point].AATSectorRadius)*2-1;
      } else {
        if (fabs(bearing)>90.0) {
          distance = -distance;
          bearing = AngleLimit180(bearing+180);
        }
        Range = distance/task_points[target_point].AATCircleRadius;
      }
      task_points[target_point].AATTargetOffsetRadius = Range;
      task_points[target_point].AATTargetOffsetRadial = bearing;
      Radial = bearing;
      SetTargetModified();
    }
  }
  mutexTaskData.Unlock();
}


static void DragTarget(double target_longitude, double target_latitude) {
  if (!AATEnabled) return;
  if (target_point==0) return;
  if (!ValidTaskPoint(target_point)) return;
  if (!ValidTaskPoint(target_point+1)) return;
  if (target_point < ActiveWayPoint) return;

  mutexTaskData.Lock();

  double distance, bearing;

  if (InAATTurnSector(target_longitude, target_latitude, target_point)) {
    if (XCSoarInterface::Calculated().IsInSector && (target_point == ActiveWayPoint)) {
      // set range/radial for inside sector
      double course_bearing, target_bearing;
      DistanceBearing(task_points[target_point-1].AATTargetLat,
                      task_points[target_point-1].AATTargetLon,
                      XCSoarInterface::Basic().Latitude,
                      XCSoarInterface::Basic().Longitude,
                      NULL, &course_bearing);

      DistanceBearing(XCSoarInterface::Basic().Latitude,
                      XCSoarInterface::Basic().Longitude,
                      target_latitude,
                      target_longitude,
                      &distance, &target_bearing);
      bearing = AngleLimit180(target_bearing-course_bearing);

      if (fabs(bearing)<90.0) {
        task_points[target_point].AATTargetLat = target_latitude;
        task_points[target_point].AATTargetLon = target_longitude;
        Radial = bearing;
        task_points[target_point].AATTargetOffsetRadial = Radial;
        Range =
          FindInsideAATSectorRange(XCSoarInterface::Basic().Latitude,
                                   XCSoarInterface::Basic().Longitude,
                                   target_point,
                                   target_bearing,
                                   distance);
        task_points[target_point].AATTargetOffsetRadius = Range;
	SetTargetModified();
      }
    } else {
      // OK to change it..
      task_points[target_point].AATTargetLat = target_latitude;
      task_points[target_point].AATTargetLon = target_longitude;

      // set range/radial for outside sector
      DistanceBearing(WayPointList[task_points[target_point].Index].Latitude,
                      WayPointList[task_points[target_point].Index].Longitude,
                      task_points[target_point].AATTargetLat,
                      task_points[target_point].AATTargetLon,
                      &distance, &bearing);
      bearing = AngleLimit180(bearing-task_points[target_point].Bisector);
      if(task_points[target_point].AATType == SECTOR) {
        Range = (fabs(distance)/task_points[target_point].AATSectorRadius)*2-1;
      } else {
        if (fabs(bearing)>90.0) {
          distance = -distance;
          bearing = AngleLimit180(bearing+180);
        }
        Range = distance/task_points[target_point].AATCircleRadius;
      }
      task_points[target_point].AATTargetOffsetRadius = Range;
      task_points[target_point].AATTargetOffsetRadial = bearing;
      Radial = bearing;
      SetTargetModified();
    }
  }
  mutexTaskData.Unlock();
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
	(void)lParam;
	(void)Sender;
  switch(wParam & 0xffff){
    case '2':
#ifdef GNAV
    case VK_F2:
#endif
      MoveTarget(0);
    return(0);
    case '3':
#ifdef GNAV
    case VK_F3:
#endif
      MoveTarget(180);
    return(0);
    case '6':
      MoveTarget(270);
    return(0);
    case '7':
      MoveTarget(90);
    return(0);
  }
  if (TargetMoveMode) {
    StartupStore(TEXT("moving\n"));
    switch(wParam & 0xffff){
    case VK_UP:
      MoveTarget(0);
      return(0);
    case VK_DOWN:
      MoveTarget(180);
      return(0);
    case VK_LEFT:
      MoveTarget(270);
      return(0);
    case VK_RIGHT:
      MoveTarget(90);
      return(0);
    }
  }
  return(1);
}



static void RefreshCalculator(void) {
  WndProperty* wp;

  RefreshTask();
  RefreshTaskStatistics();
  target_point = max(target_point,ActiveWayPoint);

  bool nodisplay = !AATEnabled
    || (target_point==0)
    || !ValidTaskPoint(target_point+1);

  if (btnMove) {
    if (nodisplay) {
      btnMove->SetVisible(false);
      TargetMoveMode = false;
    } else {
      btnMove->SetVisible(true);
    }
  }

  nodisplay = nodisplay || TargetMoveMode;

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskPoint"));
  if (wp) {
    if (TargetMoveMode) {
      wp->SetVisible(false);
    } else {
      wp->SetVisible(true);
    }
  }

  WindowControl* wc = (WindowControl*)wf->FindByName(TEXT("btnOK"));
  if (wc) {
    if (TargetMoveMode) {
      wc->SetVisible(false);
    } else {
      wc->SetVisible(true);
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAATTargetLocked"));
  if (wp) {
    wp->GetDataField()->Set(task_points[target_point].AATTargetLocked);
    wp->RefreshDisplay();
    if (nodisplay) {
      wp->SetVisible(false);
    } else {
      wp->SetVisible(true);
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Range*100.0);
    wp->RefreshDisplay();
    if (nodisplay) {
      wp->SetVisible(false);
    } else {
      wp->SetVisible(true);
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRadial"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Radial);
    wp->RefreshDisplay();
    if (nodisplay) {
      wp->SetVisible(false);
    } else {
      wp->SetVisible(true);
    }
  }

  // update outputs
  double dd = XCSoarInterface::Calculated().TaskTimeToGo;
  if ((XCSoarInterface::Calculated().TaskStartTime>0.0)&&(XCSoarInterface::Calculated().Flying)) {
    dd += XCSoarInterface::Basic().Time-XCSoarInterface::Calculated().TaskStartTime;
  }
  dd= min(24.0*60.0,dd/60.0);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEst"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(dd);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATDelta"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(dd-AATTaskLength);
    if (AATEnabled) {
      wp->SetVisible(true);
    } else {
      wp->SetVisible(false);
    }
    wp->RefreshDisplay();
  }

  double v1;
  if (XCSoarInterface::Calculated().TaskTimeToGo>0) {
    v1 = XCSoarInterface::Calculated().TaskDistanceToGo/
      XCSoarInterface::Calculated().TaskTimeToGo;
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
    wp->GetDataField()->SetAsFloat(XCSoarInterface::Calculated().TaskSpeed*TASKSPEEDMODIFY);
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

}


static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  double lon, lat;
  if (XCSoarInterface::main_window.map.TargetDragged(&lon, &lat)) {
    DragTarget(lon, lat);
  }
  if (isTargetModified()) {
    RefreshCalculator();
  }
  return 0;
}


static void OnMoveClicked(WindowControl * Sender){
  (void)Sender;
  TargetMoveMode = !TargetMoveMode;
  if (TargetMoveMode) {
    btnMove->SetCaption(TEXT("Cursor"));
  } else {
    btnMove->SetCaption(TEXT("Move"));
  }
  RefreshCalculator();
}


static void OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode) {
  double RangeNew;
  bool updated = false;
  switch(Mode){
    case DataField::daGet:
      //      Sender->Set(Range*100.0);
    break;
    case DataField::daPut:
    case DataField::daChange:
      mutexTaskData.Lock();
      if (target_point>=ActiveWayPoint) {
        RangeNew = Sender->GetAsFloat()/100.0;
        if (RangeNew != Range) {
          task_points[target_point].AATTargetOffsetRadius = RangeNew;
          Range = RangeNew;
          updated = true;
        }
      }
      mutexTaskData.Unlock();
      if (updated) {
	SetTargetModified();
      }
    break;
  }
}


static void OnRadialData(DataField *Sender, DataField::DataAccessKind_t Mode) {
  double RadialNew;
  bool updated = false;
  bool dowrap = false;
  switch(Mode){
    case DataField::daGet:
      //      Sender->Set(Range*100.0);
    break;
    case DataField::daPut:
    case DataField::daChange:
      mutexTaskData.Lock();
      if (target_point>=ActiveWayPoint) {
        if (!XCSoarInterface::Calculated().IsInSector || (target_point != ActiveWayPoint)) {
          dowrap = true;
        }
        RadialNew = Sender->GetAsFloat();
        if (fabs(RadialNew)>90) {
          if (dowrap) {
            RadialNew = AngleLimit180(RadialNew+180);
            // flip!
            Range = -Range;
            task_points[target_point].AATTargetOffsetRadius =
              -task_points[target_point].AATTargetOffsetRadius;
            updated = true;
          } else {
            RadialNew = max(-90,min(90,RadialNew));
            updated = true;
          }
        }
        if (RadialNew != Radial) {
          task_points[target_point].AATTargetOffsetRadial = RadialNew;
          Radial = RadialNew;
          updated = true;
        }
      }
      mutexTaskData.Unlock();
      if (updated) {
	SetTargetModified();
      }
    break;
  }
}


static void RefreshTargetPoint(void) {
  mutexTaskData.Lock();
  target_point = max(target_point, ActiveWayPoint);
  if (ValidTaskPoint(target_point)) {
    XCSoarInterface::SetSettingsMap().TargetPanIndex = target_point;
    XCSoarInterface::SetSettingsMap().TargetPan = true;
    Range = task_points[target_point].AATTargetOffsetRadius;
    Radial = task_points[target_point].AATTargetOffsetRadial;
  } else {
    Range = 0;
    Radial = 0;
  }
  mutexTaskData.Unlock();
  RefreshCalculator();
}


static void OnLockedData(DataField *Sender, DataField::DataAccessKind_t Mode) {
  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    case DataField::daChange:
      bool lockedthis = Sender->GetAsBoolean();
      if (ValidTaskPoint(target_point)) {
        if (task_points[target_point].AATTargetLocked !=
            lockedthis) {
	  SetTargetModified();
          task_points[target_point].AATTargetLocked = lockedthis;
        }
      }
    break;
  }
}


static void OnTaskPointData(DataField *Sender, DataField::DataAccessKind_t Mode) {
  int old_target_point = target_point;
  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    case DataField::daChange:
      target_point = Sender->GetAsInteger() + ActiveWayPointOnEntry;
      target_point = max(target_point,ActiveWayPoint);
      if (target_point != old_target_point) {
        RefreshTargetPoint();
      }
    break;
  }
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnTaskPointData),
  DeclareCallBackEntry(OnRangeData),
  DeclareCallBackEntry(OnRadialData),
  DeclareCallBackEntry(OnLockedData),
  DeclareCallBackEntry(OnOKClicked),
  DeclareCallBackEntry(OnMoveClicked),
  DeclareCallBackEntry(NULL)
};


void dlgTarget(void) {

  if (!ValidTaskPoint(ActiveWayPoint)) {
    return;
  }
  ActiveWayPointOnEntry = ActiveWayPoint;

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgTarget_L.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_TARGET_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgTarget.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_TARGET"));
  }

  if (!wf) return;

  targetManipEvent.trigger();
  TargetMoveMode = false;

  if (InfoBoxLayout::landscape)
  {// make flush right in landscape mode (at top in portrait mode)
    WndFrame *wf2 = (WndFrame*)wf->FindByName(TEXT("frmTarget"));
    if (wf2)
    {
      RECT MapRectBig = XCSoarInterface::main_window.map.GetMapRectBig();
      wf->SetLeft(MapRectBig.right- wf2->GetWidth());
    }
  }

  btnMove = (WindowControl*)wf->FindByName(TEXT("btnMove"));

  wf->SetKeyDownNotify(FormKeyDown);

  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskPoint"));
  DataFieldEnum* dfe;
  dfe = (DataFieldEnum*)wp->GetDataField();
  TCHAR tp_label[80];
  TCHAR tp_short[21];
  mutexTaskData.Lock();
  if (!ValidTaskPoint(target_point)) {
    target_point = ActiveWayPointOnEntry;
  } else {
    target_point = max(target_point, ActiveWayPointOnEntry);
  }
  for (int i=ActiveWayPointOnEntry; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(i)) {
      _tcsncpy(tp_short, WayPointList[task_points[i].Index].Name, 20);
      tp_short[20] = 0;
      _stprintf(tp_label, TEXT("%d %s"), i, tp_short);
      dfe->addEnumText(tp_label);
    } else {
      if (target_point>= i) {
        target_point= ActiveWayPointOnEntry;
      }
    }
  }
  dfe->Set(max(0,target_point-ActiveWayPointOnEntry));
  mutexTaskData.Unlock();
  wp->RefreshDisplay();

  RefreshTargetPoint();

  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal(true); // enable map

  XCSoarInterface::SetSettingsMap().TargetPan = false;

  targetManipEvent.reset();

  delete wf;
  wf = NULL;
}
