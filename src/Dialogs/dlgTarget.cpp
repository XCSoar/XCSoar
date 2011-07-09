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
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Components.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Task/Tasks/AbstractTask.hpp"
#include "Units/Units.hpp"
#include "Form/SymbolButton.hpp"
#include "Asset.hpp"

#include <stdio.h>

using std::min;
using std::max;

static WndForm *wf = NULL;
static WndButton *btnIsLocked = NULL;
static WndSymbolButton *btnNext = NULL;
static unsigned ActiveTaskPointOnEntry = 0;
static unsigned TaskSize = 0;

static fixed Range = fixed_zero;
static fixed Radial = fixed_zero;
static unsigned target_point = 0;
static bool IsLocked = true;

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
  bool bAAT = protected_task_manager->has_target(target_point);

  if (btnIsLocked)
    btnIsLocked->set_enabled(bAAT);

  if (!bAAT) {
    nodisplay = true;
    IsLocked = false;
  } else {
    protected_task_manager->get_target_range_radial(target_point, Range, Radial);
    IsLocked = protected_task_manager->target_is_locked(target_point);
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

  if (btnIsLocked) {
    btnIsLocked->SetCaption(IsLocked ? _T("Unlock") : _T("Lock"));
    btnIsLocked->set_visible(!nodisplay);
  }

  // update outputs
  fixed speedach = XCSoarInterface::Calculated().task_stats.total.travelled.get_speed();

  fixed aattimeEst = XCSoarInterface::Calculated().common_stats.task_time_remaining +
      XCSoarInterface::Calculated().common_stats.task_time_elapsed;
  fixed aatTime = protected_task_manager->get_ordered_task_behaviour().aat_min_time;

  wp = (WndProperty*)wf->FindByName(_T("prpAATEst"));// Same as infobox
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set(aattimeEst / fixed(60));
    wp->RefreshDisplay();
    wp->set_visible(!nodisplay);
  }
  wp = (WndProperty*)wf->FindByName(_T("prpAATDelta")); // same as infobox
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set((aattimeEst - aatTime) / fixed(60));
    wp->RefreshDisplay();
    wp->set_visible(!nodisplay);
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
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserTaskSpeed(speedach));
    df.SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  RefreshCalculator();
}

static void
OnIsLockedClicked(gcc_unused WndButton &Sender)
{
  IsLocked = !IsLocked;
  protected_task_manager->target_lock(target_point, IsLocked);
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
OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)Sender;

  switch (Mode) {
  case DataField::daChange:
    if (target_point >= ActiveTaskPointOnEntry) {
      const fixed RangeNew = df.GetAsFixed() / fixed(100);
      if (RangeNew != Range) {
        protected_task_manager->set_target(target_point, RangeNew, Radial);
        protected_task_manager->get_target_range_radial(target_point, Range, Radial);
      }
    }
    break;

  case DataField::daInc:
  case DataField::daDec:
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
        protected_task_manager->set_target(target_point, Range, RadialNew);
        Radial = RadialNew;
      }
    }
    break;

  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    return;
  }
}

static void
SetZoom(void)
{
  const fixed Radius =
    std::max(protected_task_manager->get_ordered_taskpoint_radius(target_point) * fixed(1.5),
             fixed(10000));
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
      const GeoPoint t = protected_task_manager->get_ordered_taskpoint_location(
              target_point,
              XCSoarInterface::Basic().Location);
      if (t == XCSoarInterface::Basic().Location)
        return; // should not happen

      XCSoarInterface::SetSettingsMap().PanLocation = t;
      XCSoarInterface::SetSettingsMap().TargetPanIndex = target_point;
    }

    fixed range = fixed_zero;
    fixed radial = fixed_zero;
    protected_task_manager->get_target_range_radial(target_point, range, radial);
    SetZoom();
    RefreshCalculator();

    ActionInterface::SendSettingsMap(true);
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

  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    return;
  }
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnTaskPointData),
  DeclareCallBackEntry(OnRangeData),
  DeclareCallBackEntry(OnRadialData),
  DeclareCallBackEntry(OnOKClicked),
  DeclareCallBackEntry(OnIsLockedClicked),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(NULL)
};

static void
GetTaskData()
{
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
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
    CopyString(tp_short, protected_task_manager->get_ordered_taskpoint_name(i),
               20);
    _stprintf(tp_label, _T("%d %s"), i, tp_short);
    dfe->addEnumText(tp_label);
  }
  dfe->Set(max(0, (int)target_point - (int)ActiveTaskPointOnEntry));

  if (TaskSize > target_point) {
    const GeoPoint t = protected_task_manager->get_ordered_taskpoint_location(target_point,
        XCSoarInterface::Basic().Location);
    SetZoom();
    XCSoarInterface::SetSettingsMap().TargetPan = true;
    XCSoarInterface::SetSettingsMap().EnablePan = true;
    XCSoarInterface::SetSettingsMap().PanLocation = t;
    XCSoarInterface::SetSettingsMap().TargetPanIndex = target_point;
    ActionInterface::SendSettingsMap(true);
  }
  wp->RefreshDisplay();
}

static void
drawBtnNext()
{
  btnNext = (WndSymbolButton*)wf->FindByName(_T("btnNext"));
  assert(btnNext != NULL);

  WndProperty * wpTaskPoint = (WndProperty*)wf->FindByName(_T("prpTaskPoint"));
  assert(wpTaskPoint != NULL);

  if (is_altair()) { // altair already has < and > buttons on WndProperty
    btnNext->set_visible(false);
  } else {
    const PixelRect rcP = wpTaskPoint->get_position();
    wpTaskPoint->move(rcP.left, rcP.top,
                      rcP.right - rcP.left - btnNext->get_width() - 1,
                      rcP.bottom - rcP.top);
    const PixelRect rcB = btnNext->get_position();
    btnNext->move(wpTaskPoint->get_right() + 1, rcB.top);
  }
}

void
dlgTargetShowModal(int TargetPoint)
{
  if (protected_task_manager == NULL ||
      protected_task_manager->get_mode() != TaskManager::MODE_ORDERED)
    return;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_TARGET_L") :
                                      _T("IDR_XML_TARGET"));
  if (!wf)
    return;

  bool oldEnablePan = XCSoarInterface::SetSettingsMap().EnablePan;
  GeoPoint oldPanLocation = XCSoarInterface::SetSettingsMap().PanLocation;
  const bool oldFullScreen = XCSoarInterface::main_window.GetFullScreen();

  if (TargetPoint >=0)
    target_point = TargetPoint;
  InitTargetPoints();

  btnIsLocked = (WndButton*)wf->FindByName(_T("btnIsLocked"));
  assert(btnIsLocked != NULL);

  drawBtnNext();

  wf->SetKeyDownNotify(FormKeyDown);

  wf->SetTimerNotify(OnTimerNotify);

  XCSoarInterface::main_window.SetFullScreen(true);

  PixelRect dialog_rect = wf->get_position();
  PixelRect map_rect = XCSoarInterface::main_window.get_client_rect();
  if (Layout::landscape)
    map_rect.left = dialog_rect.right;
  else
    map_rect.top = dialog_rect.bottom;
  XCSoarInterface::main_window.SetCustomView(map_rect);

  wf->ShowModal(XCSoarInterface::main_window.map); // enable map

  XCSoarInterface::SetSettingsMap().EnablePan = oldEnablePan;
  XCSoarInterface::SetSettingsMap().PanLocation = oldPanLocation;
  XCSoarInterface::SetSettingsMap().TargetPan = false;
  XCSoarInterface::main_window.SetFullScreen(oldFullScreen);

  XCSoarInterface::main_window.LeaveCustomView();

  delete wf;
  wf = NULL;
}
