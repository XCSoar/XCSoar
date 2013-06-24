/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "TaskDialogs.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/SymbolButton.hpp"
#include "Form/CheckBox.hpp"
#include "Form/DataField/Float.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "MapWindow/TargetMapWindow.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Units/Units.hpp"
#include "Asset.hpp"
#include "Blackboard/RateLimitedBlackboardListener.hpp"
#include "Interface.hpp"
#include "Util/Clamp.hpp"

#include <stdio.h>

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
  virtual void OnTaskModified() override;
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
    distance = std::max(tp.AATSectorRadius/20.0,distance);
  } else {
    distance = std::max(tp.AATCircleRadius/20.0,distance);
  }

  // JMW illegal
  bearing = AngleLimit360(CommonInterface::main_window.map.GetDisplayAngle()
                          + adjust_angle);

  FindLatitudeLongitude (tp.AATTargetLocation,
                         bearing, distance,
                         &target_location);

  if (task.InAATTurnSector(target_location,
                           target_point)) {
    if (CommonInterface::Calculated().IsInSector
        && (target_point == task.getActiveIndex())) {
      // set range/radial for inside sector
      double course_bearing, target_bearing;
      DistanceBearing(task.getTargetLocation(target_point-1),
                      CommonInterface::Basic().Location,
                      NULL, &course_bearing);

      DistanceBearing(CommonInterface::Basic().Location,
                      target_location,
                      &distance, &target_bearing);
      bearing = AngleLimit180(target_bearing-course_bearing);

      if (fabs(bearing)<90.0) {
        tp.AATTargetLocation = target_location;
        Radial = bearing;
        tp.AATTargetOffsetRadial = Radial;
        Range =
          task.FindInsideAATSectorRange(CommonInterface::Basic().Location,
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
    const OrderedTask &task = lease->GetOrderedTask();
    const AATPoint *ap = task.GetAATTaskPoint(target_point);

    is_aat = ap != nullptr;

    if (!is_aat) {
      nodisplay = true;
      is_locked = false;
    } else {
      range_and_radial = ap->GetTargetRangeRadial(range_and_radial.range);
      is_locked = ap->IsTargetLocked();
    }

    aat_time = task.GetOrderedTaskSettings().aat_min_time;
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
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const fixed aat_time_estimated = task_stats.GetEstimatedTotalTime();

  ShowOptionalFormControl(*wf, _T("prpAATEst"), !nodisplay);
  ShowFormControl(*wf, _T("prpAATDelta"), !nodisplay);
  if (!nodisplay) {
    LoadOptionalFormProperty(*wf, _T("prpAATEst"),
                             aat_time_estimated / fixed(60));
    LoadFormProperty(*wf, _T("prpAATDelta"),
                     (aat_time_estimated - aat_time) / 60);
  }

  const ElementStat &total = task_stats.total;
  if (total.remaining_effective.IsDefined())
    LoadFormProperty(*wf, _T("prpSpeedRemaining"), UnitGroup::TASK_SPEED,
                     total.remaining_effective.GetSpeed());

  if (total.travelled.IsDefined())
    LoadOptionalFormProperty(*wf, _T("prpSpeedAchieved"), UnitGroup::TASK_SPEED,
                             total.travelled.GetSpeed());
}

static void
UpdateNameButton()
{
  StaticString<80u> buffer;

  {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    const OrderedTask &task = lease->GetOrderedTask();
    if (target_point < task.TaskSize()) {
      const OrderedTaskPoint &tp = task.GetTaskPoint(target_point);
      buffer.Format(_T("%u: %s"), target_point,
                    tp.GetWaypoint().name.c_str());
    } else
      buffer.clear();
  }

  WndButton &name_button = *(WndButton *)wf->FindByName(_T("Name"));
  name_button.SetText(buffer);
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
OnNextClicked()
{
  if (target_point < (task_size - 1))
    target_point++;
  else
    target_point = initial_active_task_point;

  UpdateNameButton();
  RefreshTargetPoint();
}

static void
OnPrevClicked()
{
  if (target_point > initial_active_task_point)
    target_point--;
  else
    target_point = task_size - 1;

  UpdateNameButton();
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
    const OrderedTask &task = lease->GetOrderedTask();
    AATPoint *ap = task.GetAATTaskPoint(target_point);
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
  } else if (new_radial <= -Angle::HalfCircle()) {
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
    const OrderedTask &task = lease->GetOrderedTask();
    AATPoint *ap = task.GetAATTaskPoint(target_point);
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

static void
OnNameClicked()
{
  Waypoint waypoint;

  {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    const OrderedTask &task = lease->GetOrderedTask();
    if (target_point >= task.TaskSize())
      return;

    const OrderedTaskPoint &tp = task.GetTaskPoint(target_point);
    waypoint = tp.GetWaypoint();
  }

  dlgWaypointDetailsShowModal(waypoint);
}

static constexpr CallBackTableEntry callback_table[] = {
  DeclareCallBackEntry(OnCreateMap),
  DeclareCallBackEntry(OnNameClicked),
  DeclareCallBackEntry(OnRangeData),
  DeclareCallBackEntry(OnRadialData),
  DeclareCallBackEntry(OnOptimized),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(NULL)
};

static bool
GetTaskData()
{
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  if (task_manager->GetMode() != TaskType::ORDERED)
    return false;

  const OrderedTask &task = task_manager->GetOrderedTask();

  initial_active_task_point = task.GetActiveIndex();
  task_size = task.TaskSize();
  return true;
}

/**
 * Reads task points from the protected task manager
 * and loads the Task Point UI and initializes the pan mode on the map
 */
static bool
InitTargetPoints()
{
  if (!GetTaskData())
    return false;

  if (task_size <= target_point)
    target_point = initial_active_task_point;
  else
    target_point = std::max(target_point, initial_active_task_point);

  target_point = Clamp(int(target_point), 0, (int)task_size - 1);

  if (task_size > target_point) {
    SetTarget();
  }

  UpdateNameButton();
  return true;
}

static void
UpdateButtons()
{
  if (IsAltair()) {
    // Altair uses the rotary knob
    ShowFormControl(*wf, _T("btnNext"), false);
    ShowFormControl(*wf, _T("btnPrev"), false);
  }
}

static bool
FormKeyDown(unsigned key_code)
{
  switch (key_code) {
#ifdef GNAV
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
#endif

  case KEY_LEFT:
    OnPrevClicked();
    return true;

  case KEY_RIGHT:
    OnNextClicked();
    return true;
  }

  return false;
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

  wf->SetKeyDownFunction(FormKeyDown);

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
