/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Dialogs/Task.hpp"
#include "Dialogs/Internal.hpp"
#include "Look/Look.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "MainWindow.hpp"
#include "MapWindow/TargetMapWindow.hpp"
#include "Components.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Task/Tasks/AbstractTask.hpp"
#include "Units/Units.hpp"
#include "Form/SymbolButton.hpp"
#include "Form/CheckBox.hpp"
#include "Asset.hpp"
#include "InputEvents.hpp"

#include <stdio.h>

using std::min;
using std::max;

static WndForm *wf = NULL;
static TargetMapWindow *map;
static CheckBoxControl *chkbOptimized = NULL;
static WndSymbolButton *btnNext = NULL, *btnPrev = NULL;
static unsigned ActiveTaskPointOnEntry = 0;
static unsigned TaskSize = 0;

static fixed Range = fixed_zero;
static fixed Radial = fixed_zero;
static unsigned target_point = 0;
static bool IsLocked = true;

static Window *
OnCreateMap(ContainerWindow &parent, PixelScalar left, PixelScalar top,
            UPixelScalar width, UPixelScalar height,
            const WindowStyle style)
{
  const Look &look = *CommonInterface::main_window.look;

  map = new TargetMapWindow(look.waypoint, look.airspace,
                            look.task, look.aircraft);
  map->SetTerrain(terrain);
  map->SetTopograpgy(topography);
  map->SetAirspaces(&airspace_database);
  map->SetWaypoints(&way_points);
  map->SetTask(protected_task_manager);
  map->SetGlideComputer(glide_computer);
  map->set(parent, left, top, width, height, style);

  return map;
}

static void
OnOKClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void InitTargetPoints();
static void RefreshTargetPoint(void);

static void
MoveTarget(gcc_unused double adjust_angle)
{
  /*
  if (!task.getSettings().AATEnabled) return;
  if (target_point==0) return;
  if (!task.ValidTaskPoint(target_point)) return;
  if (!task.ValidTaskPoint(target_point+1)) return;
  if (target_point < task.getActiveIndex()) return;

  GeoPoint target_location;
  double bearing, distance;

  TASK_POINT tp = task.getTaskPoint(target_point);

  distance = 500;
  if (tp.AATType == AAT_SECTOR) {
    distance = max(tp.AATSectorRadius/20.0,distance);
  } else {
    distance = max(tp.AATCircleRadius/20.0,distance);
  }

  // JMW illegal
  bearing = AngleLimit360(XCSoarInterface::main_window.map.GetDisplayAngle()
                          + adjust_angle);

  FindLatitudeLongitude (tp.AATTargetLocation,
                         bearing, distance,
                         &target_location);

  if (task.InAATTurnSector(target_location,
                           target_point)) {
    if (XCSoarInterface::Calculated().IsInSector
        && (target_point == task.getActiveIndex())) {
      // set range/radial for inside sector
      double course_bearing, target_bearing;
      DistanceBearing(task.getTargetLocation(target_point-1),
                      XCSoarInterface::Basic().Location,
                      NULL, &course_bearing);

      DistanceBearing(XCSoarInterface::Basic().Location,
                      target_location,
                      &distance, &target_bearing);
      bearing = AngleLimit180(target_bearing-course_bearing);

      if (fabs(bearing)<90.0) {
        tp.AATTargetLocation = target_location;
        Radial = bearing;
        tp.AATTargetOffsetRadial = Radial;
        Range =
          task.FindInsideAATSectorRange(XCSoarInterface::Basic().Location,
                                        target_point,
                                        target_bearing,
                                        distance);
        tp.AATTargetOffsetRadius = Range;
        task.setTaskPoint(target_point, tp);
        task.SetTargetModified();
      }
    } else {
      // OK to change it..
      tp.AATTargetLocation = target_location;

      // set range/radial for outside sector
      DistanceBearing(task.getTaskPointLocation(target_point),
                      tp.AATTargetLocation,
                      &distance, &bearing);
      bearing = AngleLimit180(bearing-tp.Bisector);
      if (tp.AATType == AAT_SECTOR) {
        Range = (fabs(distance)/tp.AATSectorRadius)*2-1;
      } else {
        if (fabs(bearing)>90.0) {
          distance = -distance;
          bearing = AngleLimit180(bearing+180);
        }
        Range = distance/tp.AATCircleRadius;
      }
      tp.AATTargetOffsetRadius = Range;
      tp.AATTargetOffsetRadial = bearing;
      Radial = bearing;
      task.setTaskPoint(target_point, tp);
      task.SetTargetModified();
    }
  }
  */
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code) {
  case '2':
  case VK_F2:
    MoveTarget(0);
    return true;

  case '3':
  case VK_F3:
    MoveTarget(180);
    return true;

  case '6':
    MoveTarget(270);
    return true;

  case '7':
    MoveTarget(90);
    return true;
  }

  return false;
}

/* Lock target fields
 * if turnpoint does not have
 * adjustable target
 */
static void
LockCalculatorUI()
{
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpRange"));
  if (wp)
    wp->set_enabled(IsLocked);

  wp = (WndProperty*)wf->FindByName(_T("prpRadial"));
  if (wp)
    wp->set_enabled(IsLocked);
}

/*
 * Refresh UI based on location of target
 * and current task stats
 */
static void
RefreshCalculator()
{
  WndProperty* wp = NULL;
  bool nodisplay = false;
  bool bAAT;
  fixed aatTime;

  {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    bAAT = lease->HasTarget(target_point);

    if (!bAAT) {
      nodisplay = true;
      IsLocked = false;
    } else {
      lease->GetTargetRangeRadial(target_point, Range, Radial);
      IsLocked = lease->TargetIsLocked(target_point);
    }

    aatTime = lease->GetOrderedTaskBehaviour().aat_min_time;
  }

  if (chkbOptimized) {
    chkbOptimized->set_visible(bAAT);
    chkbOptimized->set_checked(!IsLocked);
  }

  LockCalculatorUI();

  wp = (WndProperty*)wf->FindByName(_T("prpRange"));
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set(Range * fixed(100));
    wp->RefreshDisplay();
    wp->set_visible(!nodisplay);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRadial"));
  if (wp) {
    fixed rTemp = Radial;
    if (rTemp < fixed(-90))
      rTemp += fixed(180);
    else if (rTemp > fixed(90))
      rTemp -= fixed(180);

    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set(rTemp);
    wp->RefreshDisplay();
    wp->set_visible(!nodisplay);
  }

  // update outputs
  fixed aattimeEst = XCSoarInterface::Calculated().common_stats.task_time_remaining +
      XCSoarInterface::Calculated().common_stats.task_time_elapsed;

  ShowOptionalFormControl(*wf, _T("prpAATEst"), !nodisplay);
  ShowFormControl(*wf, _T("prpAATDelta"), !nodisplay);
  if (!nodisplay) {
    LoadOptionalFormProperty(*wf, _T("prpAATEst"), aattimeEst / fixed(60));
    LoadFormProperty(*wf, _T("prpAATDelta"), (aattimeEst - aatTime) / 60);
  }

  const ElementStat &total = CommonInterface::Calculated().task_stats.total;
  LoadFormProperty(*wf, _T("prpSpeedRemaining"), ugTaskSpeed,
                   total.remaining_effective.get_speed());
  LoadOptionalFormProperty(*wf, _T("prpSpeedAchieved"), ugTaskSpeed,
                           total.travelled.get_speed());
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  RefreshCalculator();
}

static void
OnOptimized(CheckBoxControl &control)
{
  IsLocked = !control.get_checked();
  protected_task_manager->TargetLock(target_point, IsLocked);
  RefreshCalculator();
}

static void
OnNextClicked(gcc_unused WndButton &Sender)
{
  if (target_point < (TaskSize - 1))
    target_point++;
  else
    target_point = 0;
  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpTaskPoint"));
  DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->Set(target_point);
  RefreshTargetPoint();
  wp->RefreshDisplay();
}

static void
OnPrevClicked(gcc_unused WndButton &Sender)
{
  if (target_point > 0)
    target_point--;
  else
    target_point = TaskSize - 1;

  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpTaskPoint"));
  DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->Set(target_point);
  RefreshTargetPoint();
  wp->RefreshDisplay();
}

static void
OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)Sender;

  switch (Mode) {
  case DataField::daChange:
    if (target_point >= ActiveTaskPointOnEntry) {
      const fixed RangeNew = df.GetAsFixed() / fixed(100);
      if (RangeNew != Range) {
        ProtectedTaskManager::ExclusiveLease lease(*protected_task_manager);
        lease->SetTarget(target_point, RangeNew, Radial);
        lease->GetTargetRangeRadial(target_point, Range, Radial);
      }
    }
    break;

  case DataField::daSpecial:
    return;
  }
}

/*
 * reads Radial from the screen UI,  translates it from [90, -90]
 * to [180, -180] based on whether the Range variable is positive or negative
 */
static void
OnRadialData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)Sender;

  fixed RadialNew;
  switch (Mode) {
  case DataField::daChange:
    if (target_point >= ActiveTaskPointOnEntry) {
      fixed rTemp = df.GetAsFixed();
      if (fabs(Radial) > fixed(90)) {
        if (rTemp < fixed_zero)
          RadialNew = rTemp + fixed(180);
        else
          RadialNew = rTemp - fixed(180);
      } else {
        RadialNew = rTemp;
      }
      if (Radial != RadialNew) {
        protected_task_manager->SetTarget(target_point, Range, RadialNew);
        Radial = RadialNew;
      }
    }
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
SetTarget()
{
  map->SetTarget(target_point);
}

/* resets the target point and reads its polar coordinates
 * from the AATPoint's target
 */
static void
RefreshTargetPoint(void)
{
  if (target_point < TaskSize && target_point >= ActiveTaskPointOnEntry) {
    SetTarget();

    RefreshCalculator();
  } else {
    Range = fixed_zero;
    Radial = fixed_zero;
  }
}

/*
 * handles UI event where task point is changed
 */
static void
OnTaskPointData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  unsigned old_target_point = target_point;
  switch (Mode) {
  case DataField::daChange:
    target_point = Sender->GetAsInteger() + ActiveTaskPointOnEntry;
    if (target_point != old_target_point) {
      RefreshTargetPoint();
    }
    break;

  case DataField::daSpecial:
    return;
  }
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCreateMap),
  DeclareCallBackEntry(OnTaskPointData),
  DeclareCallBackEntry(OnRangeData),
  DeclareCallBackEntry(OnRadialData),
  DeclareCallBackEntry(OnOKClicked),
  DeclareCallBackEntry(OnOptimized),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(NULL)
};

static void
GetTaskData()
{
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  const AbstractTask *at = task_manager->GetActiveTask();

  ActiveTaskPointOnEntry = task_manager->GetActiveTaskPointIndex();
  TaskSize = at->TaskSize();
}

/*
 * Reads task points from the
 * protected task manager
 * and loads the Task Point UI
 * and initializes the pan mode on the map
 */
static void
InitTargetPoints()
{
  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpTaskPoint"));
  GetTaskData();
  DataFieldEnum* dfe;
  dfe = (DataFieldEnum*)wp->GetDataField();
  TCHAR tp_label[80];
  TCHAR tp_short[21];

  if (TaskSize <= target_point)
    target_point = ActiveTaskPointOnEntry;
  else
    target_point = max(target_point, ActiveTaskPointOnEntry);

  target_point = max(0, min((int)target_point, (int)TaskSize - 1));
  for (unsigned i = ActiveTaskPointOnEntry; i < TaskSize; i++) {
    CopyString(tp_short, protected_task_manager->GetOrderedTaskpointName(i),
               20);
    _stprintf(tp_label, _T("%d %s"), i, tp_short);
    dfe->addEnumText(tp_label);
  }
  dfe->Set(max(0, (int)target_point - (int)ActiveTaskPointOnEntry));

  if (TaskSize > target_point) {
    SetTarget();
  }

  wp->RefreshDisplay();
}

static void
drawBtnNext()
{
  btnNext = (WndSymbolButton*)wf->FindByName(_T("btnNext"));
  assert(btnNext != NULL);

  if (is_altair())
    // altair already has < and > buttons on WndProperty
    btnNext->set_visible(false);

  btnPrev = (WndSymbolButton*)wf->FindByName(_T("btnNext"));
  assert(btnPrev != NULL);

  if (is_altair())
    // altair already has < and > buttons on WndProperty
    btnPrev->set_visible(false);
}

void
dlgTargetShowModal(int TargetPoint)
{
  if (protected_task_manager == NULL ||
      protected_task_manager->GetMode() != TaskManager::MODE_ORDERED)
    return;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_TARGET_L") :
                                      _T("IDR_XML_TARGET"));
  assert(wf != NULL);

  if (TargetPoint >=0)
    target_point = TargetPoint;
  InitTargetPoints();

  chkbOptimized = (CheckBoxControl*)wf->FindByName(_T("chkbOptimized"));
  assert(chkbOptimized != NULL);

  drawBtnNext();

  wf->SetKeyDownNotify(FormKeyDown);
  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal();

  delete wf;
  map = NULL;
}

void
TargetDialogUpdate()
{
  if (map != NULL)
    map->invalidate();
}
