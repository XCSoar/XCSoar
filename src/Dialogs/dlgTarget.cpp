/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/SymbolButton.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Float.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "MapWindow/TargetMapWindow.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Units/Units.hpp"
#include "Asset.hpp"
#include "Blackboard/RateLimitedBlackboardListener.hpp"
#include "Interface.hpp"

#include <stdio.h>

using std::min;
using std::max;

class TargetDialogMapWindow : public TargetMapWindow {
public:
  TargetDialogMapWindow(const WaypointLook &waypoint_look,
                        const AirspaceLook &airspace_look,
                        const TrailLook &trail_look,
                        const TaskLook &task_look,
                        const AircraftLook &aircraft_look)
    :TargetMapWindow(waypoint_look, airspace_look, trail_look,
                     task_look, aircraft_look) {}

protected:
  virtual void OnTaskModified();
};

static WndForm *wf = NULL;
static TargetMapWindow *map;
static CheckBoxControl *optimized_checkbox = NULL;
static unsigned initial_active_task_point = 0;
static unsigned task_size = 0;

static RangeAndRadial range_and_radial;
static unsigned target_point = 0;
static bool is_locked = true;

static Window *
OnCreateMap(ContainerWindow &parent, PixelRect rc, const WindowStyle style)
{
  const MapLook &look = UIGlobals::GetMapLook();
  map = new TargetDialogMapWindow(look.waypoint, look.airspace,
                                  look.trail, look.task, look.aircraft);
  map->SetTerrain(terrain);
  map->SetTopograpgy(topography);
  map->SetAirspaces(&airspace_database);
  map->SetWaypoints(&way_points);
  map->SetTask(protected_task_manager);
  map->SetGlideComputer(glide_computer);
  map->Create(parent, rc, style);

  return map;
}

static void
OnOKClicked(gcc_unused WndButton &sender)
{
  wf->SetModalResult(mrOK);
}

static void RefreshTargetPoint();

#ifdef GNAV

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
FormKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_APP2:
    MoveTarget(0);
    return true;

  case KEY_APP3:
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

#endif /* GNAV */

/**
 * Locks target fields if turnpoint does not have adjustable target
 */
static void
LockCalculatorUI()
{
  SetFormControlEnabled(*wf, _T("prpRange"), is_locked);
  SetFormControlEnabled(*wf, _T("prpRadial"), is_locked);
}

static void
LoadRange()
{
  LoadFormProperty(*wf, _T("prpRange"), range_and_radial.range * 100);
}

static void
LoadRadial()
{
  LoadFormProperty(*wf, _T("prpRadial"), range_and_radial.radial.Degrees());
}

/**
 * Loads the #range_and_radial variable into the range/radial form
 * controls.
 */
static void
LoadRangeAndRadial()
{
  LoadRange();
  LoadRadial();
}

/**
 * Refreshes UI based on location of target and current task stats
 */
static void
RefreshCalculator()
{
  bool nodisplay = false;
  bool is_aat;
  fixed aat_time;

  {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    const AATPoint *ap = lease->GetAATTaskPoint(target_point);

    is_aat = ap != nullptr;

    if (!is_aat) {
      nodisplay = true;
      is_locked = false;
    } else {
      range_and_radial = ap->GetTargetRangeRadial(range_and_radial.range);
      is_locked = ap->IsTargetLocked();
    }

    aat_time = lease->GetOrderedTaskBehaviour().aat_min_time;
  }

  if (optimized_checkbox) {
    optimized_checkbox->SetVisible(is_aat);
    optimized_checkbox->SetState(!is_locked);
  }

  LockCalculatorUI();

  ShowOptionalFormControl(*wf, _T("prpRange"), !nodisplay);
  ShowOptionalFormControl(*wf, _T("prpRadial"), !nodisplay);

  if (!nodisplay)
    LoadRangeAndRadial();

  // update outputs
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  const fixed aat_time_estimated =
    common_stats.task_time_remaining + common_stats.task_time_elapsed;

  ShowOptionalFormControl(*wf, _T("prpAATEst"), !nodisplay);
  ShowFormControl(*wf, _T("prpAATDelta"), !nodisplay);
  if (!nodisplay) {
    LoadOptionalFormProperty(*wf, _T("prpAATEst"),
                             aat_time_estimated / fixed(60));
    LoadFormProperty(*wf, _T("prpAATDelta"),
                     (aat_time_estimated - aat_time) / 60);
  }

  const ElementStat &total = CommonInterface::Calculated().task_stats.total;
  if (total.remaining_effective.IsDefined())
    LoadFormProperty(*wf, _T("prpSpeedRemaining"), UnitGroup::TASK_SPEED,
                     total.remaining_effective.GetSpeed());

  if (total.travelled.IsDefined())
    LoadOptionalFormProperty(*wf, _T("prpSpeedAchieved"), UnitGroup::TASK_SPEED,
                             total.travelled.GetSpeed());
}

void
TargetDialogMapWindow::OnTaskModified()
{
  TargetMapWindow::OnTaskModified();
  RefreshCalculator();
}

static void
OnOptimized(CheckBoxControl &control)
{
  is_locked = !control.GetState();
  protected_task_manager->TargetLock(target_point, is_locked);
  RefreshCalculator();
}

static void
OnNextClicked(gcc_unused WndButton &sender)
{
  if (target_point < (task_size - 1))
    target_point++;
  else
    target_point = initial_active_task_point;

  LoadFormPropertyEnum(*wf, _T("prpTaskPoint"),
                       target_point - initial_active_task_point);
  RefreshTargetPoint();
}

static void
OnPrevClicked(gcc_unused WndButton &sender)
{
  if (target_point > initial_active_task_point)
    target_point--;
  else
    target_point = task_size - 1;

  LoadFormPropertyEnum(*wf, _T("prpTaskPoint"),
                       target_point - initial_active_task_point);
  RefreshTargetPoint();
}

static void
OnRangeModified(fixed new_value)
{
  if (target_point < initial_active_task_point)
    return;

  const fixed new_range = new_value / fixed(100);
  if (new_range == range_and_radial.range)
    return;

  if (negative(new_range) != negative(range_and_radial.range)) {
    /* when the range gets flipped, flip the radial as well */
    if (negative(range_and_radial.radial.Native()))
      range_and_radial.radial += Angle::HalfCircle();
    else
      range_and_radial.radial -= Angle::HalfCircle();
    LoadRadial();
  }

  range_and_radial.range = new_range;

  {
    ProtectedTaskManager::ExclusiveLease lease(*protected_task_manager);
    AATPoint *ap = lease->GetAATTaskPoint(target_point);
    if (ap == nullptr)
      return;

    ap->SetTarget(range_and_radial,
                  lease->GetOrderedTask().GetTaskProjection());
  }

  map->Invalidate();
}

static void
OnRangeData(DataField *sender, DataField::DataAccessMode mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)sender;

  switch (mode) {
  case DataField::daChange:
    OnRangeModified(df.GetAsFixed());
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnRadialModified(fixed new_value)
{
  if (target_point < initial_active_task_point)
    return;

  Angle new_radial = Angle::Degrees(new_value);
  if (new_radial == range_and_radial.radial)
    return;

  bool must_reload_radial = false;
  if (new_radial >= Angle::HalfCircle()) {
    new_radial -= Angle::FullCircle();
    must_reload_radial = true;
  } else if (new_radial <= Angle::HalfCircle()) {
    new_radial += Angle::FullCircle();
    must_reload_radial = true;
  }

  if ((new_radial.Absolute() > Angle::QuarterCircle()) !=
      (range_and_radial.radial.Absolute() > Angle::QuarterCircle())) {
    /* when the radial crosses the +/-90 degrees threshold, flip the
       range */
    range_and_radial.range = -range_and_radial.range;
    LoadRange();
  }

  range_and_radial.radial = new_radial;

  {
    ProtectedTaskManager::ExclusiveLease lease(*protected_task_manager);
    AATPoint *ap = lease->GetAATTaskPoint(target_point);
    if (ap == nullptr)
      return;

    ap->SetTarget(range_and_radial,
                  lease->GetOrderedTask().GetTaskProjection());
  }

  if (must_reload_radial)
    LoadRadial();

  map->Invalidate();
}

static void
OnRadialData(DataField *sender, DataField::DataAccessMode mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)sender;

  switch (mode) {
  case DataField::daChange:
    OnRadialModified(df.GetAsFixed());
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

/**
 * resets the target point and reads its polar coordinates
 * from the AATPoint's target
 */
static void
RefreshTargetPoint()
{
  if (target_point < task_size && target_point >= initial_active_task_point) {
    SetTarget();

    RefreshCalculator();
  } else {
    range_and_radial = RangeAndRadial::Zero();
  }
}

/**
 * handles UI event where task point is changed
 */
static void
OnTaskPointData(DataField *sender, DataField::DataAccessMode mode)
{
  unsigned old_target_point = target_point;
  switch (mode) {
  case DataField::daChange:
    target_point = sender->GetAsInteger() + initial_active_task_point;
    if (target_point != old_target_point) {
      RefreshTargetPoint();
    }
    break;

  case DataField::daSpecial:
    return;
  }
}

static constexpr CallBackTableEntry callback_table[] = {
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

static bool
GetTaskData(DataFieldEnum &df)
{
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  if (task_manager->GetMode() != TaskManager::MODE_ORDERED)
    return false;

  const OrderedTask &task = task_manager->GetOrderedTask();

  initial_active_task_point = task_manager->GetActiveTaskPointIndex();
  task_size = task.TaskSize();

  for (unsigned i = initial_active_task_point; i < task_size; i++) {
    StaticString<80u> label;
    label.Format(_T("%d %s"), i,
                 task.GetTaskPoint(i).GetWaypoint().name.c_str());
    df.addEnumText(label.c_str());
  }

  return true;
}

/**
 * Reads task points from the protected task manager
 * and loads the Task Point UI and initializes the pan mode on the map
 */
static bool
InitTargetPoints()
{
  WndProperty &wp = *(WndProperty *)wf->FindByName(_T("prpTaskPoint"));
  DataFieldEnum &dfe = *(DataFieldEnum *)wp.GetDataField();
  if (!GetTaskData(dfe))
    return false;

  if (task_size <= target_point)
    target_point = initial_active_task_point;
  else
    target_point = max(target_point, initial_active_task_point);

  target_point = max(0, min((int)target_point, (int)task_size - 1));

  dfe.Set(max(0, (int)target_point - (int)initial_active_task_point));

  if (task_size > target_point) {
    SetTarget();
  }

  wp.RefreshDisplay();
  return true;
}

static void
UpdateButtons()
{
  if (IsAltair()) {
    // altair already has < and > buttons on WndProperty
    ShowFormControl(*wf, _T("btnNext"), false);
    ShowFormControl(*wf, _T("btnPrev"), false);
  }
}

void
dlgTargetShowModal(int _target_point)
{
  if (protected_task_manager == NULL)
    return;

  wf = LoadDialog(callback_table, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_TARGET_L") :
                                      _T("IDR_XML_TARGET"));
  assert(wf != NULL);

  if (_target_point >= 0)
    target_point = _target_point;

  if (!InitTargetPoints()) {
    delete wf;
    map = NULL;
    return;
  }

  optimized_checkbox = (CheckBoxControl*)wf->FindByName(_T("chkbOptimized"));
  assert(optimized_checkbox != NULL);

  UpdateButtons();

#ifdef GNAV
  wf->SetKeyDownFunction(FormKeyDown);
#endif

  struct TargetDialogUpdateListener : public NullBlackboardListener {
    virtual void OnCalculatedUpdate(const MoreData &basic,
                                    const DerivedInfo &calculated) {
      map->Invalidate();
      RefreshCalculator();
    }
  };

  TargetDialogUpdateListener blackboard_listener;
  RateLimitedBlackboardListener rate_limited_bl(blackboard_listener,
                                                1800, 300);

  //WindowBlackboardListener
  CommonInterface::GetLiveBlackboard().AddListener(rate_limited_bl);

  wf->ShowModal();
  delete wf;

  CommonInterface::GetLiveBlackboard().RemoveListener(rate_limited_bl);
}
