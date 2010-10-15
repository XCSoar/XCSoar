/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
  Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "MainWindow.hpp"
#include "Components.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Task/Tasks/AbstractTask.hpp"
#include "Units.hpp"

#include <stdio.h>

using std::min;
using std::max;

static WndForm *wf = NULL;
static WndButton *btnMove = NULL;
static WndButton *btnIsLocked = NULL;
static unsigned ActiveTaskPointOnEntry = 0;
unsigned TaskSize = 0;

bool oldEnablePan = false;
GeoPoint oldPanLocation;

static fixed Range = fixed_zero;
static fixed Radial = fixed_zero;
static unsigned target_point = 0;
static bool TargetMoveMode = false;
static bool IsLocked = true;

static void
OnOKClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void InitTargetPoints();

static void
MoveTarget(double adjust_angle)
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
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;
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

  if (TargetMoveMode) {
    switch (key_code) {
    case VK_UP:
      MoveTarget(0);
      return true;

    case VK_DOWN:
      MoveTarget(180);
      return true;

    case VK_LEFT:
      MoveTarget(270);
      return true;

    case VK_RIGHT:
      MoveTarget(90);
      return true;
    }
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
  if (btnMove)
    btnMove->set_enabled(IsLocked);

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
  bool bAAT = protected_task_manager.has_target(target_point);

  if (btnIsLocked)
    btnIsLocked->set_enabled(bAAT);

  if (!bAAT) {
    nodisplay = true;
    IsLocked = false;
  } else {
    protected_task_manager.get_target_range_radial(target_point, Range, Radial);
    IsLocked = protected_task_manager.target_is_locked(target_point);
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

  if (btnMove) {
    btnMove->set_visible(false);
    // todo add functionality for a cursor/move button
    if (nodisplay)
      TargetMoveMode = false;
  }
  nodisplay = nodisplay || TargetMoveMode;

  wp = (WndProperty*)wf->FindByName(_T("prpTaskPoint"));
  if (wp)
    wp->set_visible(!TargetMoveMode);

  WindowControl* wc = (WindowControl*)wf->FindByName(_T("btnOK"));
  if (wc)
    wc->set_visible(!TargetMoveMode);

  if (btnIsLocked) {
    btnIsLocked->SetCaption(IsLocked ? _T("Locked") : _T("Auto"));
    btnIsLocked->set_visible(!nodisplay);
  }

  // update outputs
  fixed speedach = XCSoarInterface::Calculated().task_stats.total.travelled.get_speed();

  fixed aattimeEst = XCSoarInterface::Calculated().common_stats.task_time_remaining +
      XCSoarInterface::Calculated().common_stats.task_time_elapsed;
  fixed aatTime = protected_task_manager.get_ordered_task_behaviour().aat_min_time;

  wp = (WndProperty*)wf->FindByName(_T("prpAATEst"));// Same as infobox
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set(aattimeEst / fixed(60));
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpAATDelta")); // same as infobox
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set((aatTime - aattimeEst) / fixed(60));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSpeedRemaining"));
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set(Units::ToUserTaskSpeed(
       XCSoarInterface::Calculated().task_stats.total.remaining_effective.get_speed()));
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSpeedAchieved"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Units::ToUserUnit(speedach, Units::TaskSpeedUnit));
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }
}

static void
OnTimerNotify(WindowControl * Sender)
{
  (void)Sender;
  RefreshCalculator();
}

static void
OnMoveClicked(WindowControl * Sender)
{
  (void)Sender;
  TargetMoveMode = !TargetMoveMode;
  if (btnMove)
    btnMove->SetCaption(TargetMoveMode ? _T("Cursor") : _T("Move"));
  RefreshCalculator();
}

static void
OnIsLockedClicked(WindowControl * Sender)
{
  (void)Sender;
  IsLocked = !IsLocked;
  protected_task_manager.target_lock(target_point, IsLocked);
  RefreshCalculator();
}

static void
OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
    if (target_point >= ActiveTaskPointOnEntry) {
      const fixed RangeNew = Sender->GetAsFixed() / fixed(100);
      if (RangeNew != Range) {
        protected_task_manager.set_target(target_point, RangeNew, Radial);
        Range = RangeNew;
      }
    }
    break;
  }
}

/*
 * reads Radial from the screen UI,  translates it from [90, -90]
 * to [180, -180] based on whether the Range variable is positive or negative
 */
static void
OnRadialData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  fixed RadialNew;
  switch (Mode) {
  case DataField::daChange:
    if (target_point >= ActiveTaskPointOnEntry) {
      fixed rTemp = Sender->GetAsFixed();
      if (fabs(Radial) > fixed(90)) {
        if (rTemp < fixed_zero)
          RadialNew = rTemp + fixed(180);
        else
          RadialNew = rTemp - fixed(180);
      } else {
        RadialNew = rTemp;
      }
      if (Radial != RadialNew) {
        protected_task_manager.set_target(target_point, Range, RadialNew);
        Radial = RadialNew;
      }
    }
    break;
  }
}

static void
SetRadius(void)
{
  const fixed Radius =
      protected_task_manager.get_ordered_taskpoint_radius(target_point) * fixed(2);
  XCSoarInterface::SetSettingsMap().TargetZoomDistance = Radius;
}

/* resets the target point and reads its polar coordinates
 * from the AATPoint's target
 */
static void
RefreshTargetPoint(void)
{
  if (target_point < TaskSize && target_point >= ActiveTaskPointOnEntry) {
    if (XCSoarInterface::SetSettingsMap().TargetPanIndex != target_point) {
      const GeoPoint t = protected_task_manager.get_ordered_taskpoint_location(
              target_point,
              XCSoarInterface::Basic().Location);
      if (t == XCSoarInterface::Basic().Location)
        return; // should not happen

      XCSoarInterface::SetSettingsMap().PanLocation = t;
      XCSoarInterface::SetSettingsMap().TargetPanIndex = target_point;
    }

    fixed range = fixed_zero;
    fixed radial = fixed_zero;
    protected_task_manager.get_target_range_radial(target_point, range, radial);
    SetRadius();
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
  }
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnTaskPointData),
  DeclareCallBackEntry(OnRangeData),
  DeclareCallBackEntry(OnRadialData),
  DeclareCallBackEntry(OnOKClicked),
  DeclareCallBackEntry(OnMoveClicked),
  DeclareCallBackEntry(OnIsLockedClicked),
  DeclareCallBackEntry(NULL)
};

static void
GetTaskData()
{
  ProtectedTaskManager::Lease task_manager(protected_task_manager);
  const AbstractTask *at = task_manager->get_active_task();

  ActiveTaskPointOnEntry = task_manager->getActiveTaskPointIndex();
  TaskSize = at->task_size();
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
    _tcsncpy(tp_short, protected_task_manager.get_ordered_taskpoint_name(i), 20);
    tp_short[20] = 0;
    _stprintf(tp_label, _T("%d %s"), i, tp_short);
    dfe->addEnumText(tp_label);
  }
  dfe->Set(max(0, (int)target_point - (int)ActiveTaskPointOnEntry));

  if (TaskSize > target_point) {
    const GeoPoint t = protected_task_manager.get_ordered_taskpoint_location(target_point,
        XCSoarInterface::Basic().Location);
    SetRadius();
    XCSoarInterface::SetSettingsMap().TargetPan = true;
    XCSoarInterface::SetSettingsMap().EnablePan = true;
    XCSoarInterface::SetSettingsMap().PanLocation = t;
    XCSoarInterface::SetSettingsMap().TargetPanIndex = target_point;
  }
  wp->RefreshDisplay();
}

void
dlgTargetShowModal()
{

  if (protected_task_manager.get_mode() != TaskManager::MODE_ORDERED)
    return;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_TARGET") :
                                      _T("IDR_XML_TARGET_L"));
  if (!wf)
    return;

  oldEnablePan = XCSoarInterface::SetSettingsMap().EnablePan;
  oldPanLocation = XCSoarInterface::SetSettingsMap().PanLocation;

  InitTargetPoints();

  TargetMoveMode = false;

  btnMove = (WndButton*)wf->FindByName(_T("btnMove"));
  if (btnMove)
    btnMove->set_visible(false); // todo enable move buttons
  btnIsLocked = (WndButton*)wf->FindByName(_T("btnIsLocked"));

  assert(btnIsLocked != NULL);

  wf->SetKeyDownNotify(FormKeyDown);

  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal(true); // enable map
  XCSoarInterface::SetSettingsMap().EnablePan = oldEnablePan;
  XCSoarInterface::SetSettingsMap().PanLocation = oldPanLocation;
  XCSoarInterface::SetSettingsMap().TargetPan = false;

  delete wf;
  wf = NULL;
}
