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

#include "InfoBoxes/Content/Task.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "MainWindow.hpp"
#include "LocalTime.hpp"
#include "Engine/Util/Gradient.hpp"
#include "Units/UnitsFormatter.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentBearing::Update(InfoBoxData &data)
{
  const GlideResult &solution_remaining =
    XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining;
  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !solution_remaining.IsDefined() ||
      solution_remaining.vector.distance <= fixed(10)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(solution_remaining.vector.bearing, _T("T"));
}

void
InfoBoxContentBearingDiff::Update(InfoBoxData &data)
{
  const GlideResult &solution_remaining =
    XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining;
  if (!XCSoarInterface::Basic().track_available ||
      !XCSoarInterface::Calculated().task_stats.task_valid ||
      !solution_remaining.IsOk() ||
      solution_remaining. vector.distance <= fixed(10)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  Angle Value =
    solution_remaining.vector.bearing - XCSoarInterface::Basic().track;

  SetValueBearingDifference(data, Value);
}

void
InfoBoxContentNextWaypoint::Update(InfoBoxData &data)
{
  // use proper non-terminal next task stats

  const Waypoint* way_point = protected_task_manager != NULL
    ? protected_task_manager->GetActiveWaypoint()
    : NULL;

  if (!way_point) {
    data.SetTitle(_("Next"));
    data.SetInvalid();
    return;
  }
  SetTitleFromWaypointName(data, way_point);

  // Set Comment
  if (way_point->radio_frequency.IsDefined()) {
    const unsigned freq = way_point->radio_frequency.GetKiloHertz();
    data.FormatComment(_T("%u.%03u %s"),
                       freq / 1000, freq % 1000, way_point->comment.c_str());
  }
  else
    data.SetComment(way_point->comment.c_str());

  const GlideResult &solution_remaining =
    XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining;
  if (!XCSoarInterface::Basic().track_available ||
      !XCSoarInterface::Calculated().task_stats.task_valid ||
      !solution_remaining.IsDefined() ||
      solution_remaining.vector.distance <= fixed(10)) {
    data.SetValueInvalid();
    return;
  }

  // Set Value
  Angle Value =
    solution_remaining.vector.bearing - XCSoarInterface::Basic().track;

  SetValueBearingDifference(data, Value);

  // Set Color (blue/black)
  data.SetValueColor(solution_remaining.IsFinalGlide() ? 2 : 0);
}

bool
InfoBoxContentNextWaypoint::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (protected_task_manager == NULL)
    return false;

  switch (keycode) {
  case ibkRight:
  case ibkUp:
    protected_task_manager->IncrementActiveTaskPoint(1);
    return true;

  case ibkLeft:
  case ibkDown:
    protected_task_manager->IncrementActiveTaskPoint(-1);
    return true;

  case ibkEnter:
    const Waypoint *wp = protected_task_manager->GetActiveWaypoint();
    if (wp) {
      dlgWaypointDetailsShowModal(XCSoarInterface::main_window, *wp);
      return true;
    }
  }

  return false;
}

void
InfoBoxContentNextDistance::Update(InfoBoxData &data)
{
  // use proper non-terminal next task stats

  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const GlideResult &solution_remaining =
    XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining;
  if (!task_stats.task_valid ||
      !solution_remaining.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(data, solution_remaining.vector.distance);

  if (XCSoarInterface::Basic().track_available) {
    Angle bd = solution_remaining.vector.bearing - XCSoarInterface::Basic().track;
    SetCommentBearingDifference(data, bd);
  } else
    data.SetCommentInvalid();
}

void
InfoBoxContentNextETE::Update(InfoBoxData &data)
{
  // use proper non-terminal next task stats

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !XCSoarInterface::Calculated().task_stats.current_leg.IsAchievable() ||
      !positive(XCSoarInterface::Calculated().task_stats.
                current_leg.time_remaining)) {
    data.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = (int)XCSoarInterface::Calculated().
      task_stats.current_leg.time_remaining;
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  data.SetValue(HHMMSSsmart);
  data.SetComment(SSsmart);
}

void
InfoBoxContentNextETA::Update(InfoBoxData &data)
{
  // use proper non-terminal next task stats

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !XCSoarInterface::Calculated().task_stats.current_leg.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  int dd = (int)(XCSoarInterface::Calculated().task_stats.current_leg.
                 solution_remaining.time_elapsed) +
    DetectCurrentTime(XCSoarInterface::Basic());
  const BrokenTime t = BrokenTime::FromSecondOfDayChecked(abs(dd));

  // Set Value
  data.UnsafeFormatValue(_T("%02u:%02u"), t.hour, t.minute);

  // Set Comment
  data.UnsafeFormatComment(_T("%02u"), t.second);
}

void
InfoBoxContentNextAltitudeDiff::Update(InfoBoxData &data)
{
  // pilots want this to be assuming terminal flight to this wp

  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const GlideResult &next_solution = XCSoarInterface::Calculated().common_stats.next_solution;
  if (!task_stats.task_valid || !next_solution.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(next_solution.altitude_difference, tmp, 32, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxContentNextAltitudeRequire::Update(InfoBoxData &data)
{
  // pilots want this to be assuming terminal flight to this wp

  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const GlideResult &next_solution = XCSoarInterface::Calculated().common_stats.next_solution;
  if (!task_stats.task_valid || !next_solution.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(next_solution.altitude_required, tmp, 32, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxContentNextAltitudeArrival::Update(InfoBoxData &data)
{
  // pilots want this to be assuming terminal flight to this wp

  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const GlideResult next_solution = XCSoarInterface::Calculated().common_stats.next_solution;
  if (!task_stats.task_valid || !next_solution.IsAchievable(true)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  fixed alt = XCSoarInterface::Basic().nav_altitude-next_solution.height_glide;
  Units::FormatUserAltitude(alt, tmp, 32, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.altitude_unit);
}


void
InfoBoxContentNextLD::Update(InfoBoxData &data)
{
  // pilots want this to be assuming terminal flight to this wp, and this
  // is what current_leg gradient does.

  if (!XCSoarInterface::Calculated().task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  fixed gradient = XCSoarInterface::Calculated().task_stats.current_leg.gradient;

  if (!positive(gradient)) {
    data.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    data.UnsafeFormatValue(_T("%d"), (int)gradient);
  } else {
    data.SetInvalid();
  }
}

void
InfoBoxContentFinalDistance::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!task_stats.task_valid ||
      !task_stats.current_leg.solution_remaining.IsDefined()) {
    data.SetInvalid();
    return;
  }

  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  // Set Value
  SetValueFromDistance(data, common_stats.task_finished ?
                                task_stats.current_leg.solution_remaining.vector.distance :
                                task_stats.total.remaining.get_distance());
}

void
InfoBoxContentFinalETE::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!task_stats.task_valid || !task_stats.total.IsAchievable() ||
      !positive(task_stats.total.time_remaining)) {
    data.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = abs((int)task_stats.total.time_remaining);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  data.SetValue(HHMMSSsmart);
  data.SetComment(SSsmart);
}

void
InfoBoxContentFinalETA::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.total.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  int dd = (int)task_stats.total.solution_remaining.time_elapsed +
    DetectCurrentTime(XCSoarInterface::Basic());
  const BrokenTime t = BrokenTime::FromSecondOfDayChecked(abs(dd));

  // Set Value
  data.UnsafeFormatValue(_T("%02u:%02u"), t.hour, t.minute);

  // Set Comment
  data.UnsafeFormatComment(_T("%02u"), t.second);
}

void
InfoBoxContentFinalAltitudeDiff::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid ||
      !task_stats.total.solution_remaining.IsOk()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(task_stats.total.solution_remaining.altitude_difference,
                            tmp, 32, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxContentFinalAltitudeRequire::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid ||
      !task_stats.total.solution_remaining.IsOk()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(task_stats.total.solution_remaining.altitude_required,
                            tmp, 32, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxContentTaskSpeed::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.total.travelled.get_speed()));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
InfoBoxContentTaskSpeedAchieved::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.total.remaining_effective.get_speed()));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
InfoBoxContentTaskSpeedInstant::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.get_pirker_speed()));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
InfoBoxContentFinalLD::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  fixed gradient = task_stats.total.gradient;

  if (!positive(gradient)) {
    data.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    data.UnsafeFormatValue(_T("%d"), (int)gradient);
  } else {
    data.SetInvalid();
  }
}

void
InfoBoxContentFinalGR::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  fixed gradient = task_stats.total.gradient;

  if (!positive(gradient)) {
    data.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    data.UnsafeFormatValue(_T("%d"), (int)gradient);
  } else {
    data.SetInvalid();
  }
}

void
InfoBoxContentHomeDistance::Update(InfoBoxData &data)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!common_stats.vector_home.IsValid()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(data, common_stats.vector_home.distance);

  if (XCSoarInterface::Basic().track_available) {
    Angle bd = common_stats.vector_home.bearing - XCSoarInterface::Basic().track;
    SetCommentBearingDifference(data, bd);
  } else
    data.SetCommentInvalid();
}

void
InfoBoxContentOLC::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::SettingsComputer().task.enable_olc ||
      !protected_task_manager) {
    data.SetInvalid();
    return;
  }

  const ContestResult& result_olc = 
    XCSoarInterface::Calculated().contest_stats.GetResult();

  if (result_olc.score < fixed_one) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(data, result_olc.distance);

  data.UnsafeFormatComment(_T("%.1f pts"), (double)result_olc.score);
}

bool
InfoBoxContentOLC::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    dlgAnalysisShowModal(XCSoarInterface::main_window,
                         *CommonInterface::main_window.look,
                         CommonInterface::Full(), *glide_computer,
                         protected_task_manager, &airspace_database, terrain,
                         8);
    return true;

  default:
    break;
  }

  return false;
}

void
InfoBoxContentTaskAATime::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!common_stats.ordered_has_targets ||
      !task_stats.task_valid || !task_stats.total.IsAchievable() ||
      !positive(common_stats.aat_time_remaining)) {
    data.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = abs((int)common_stats.aat_time_remaining);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  data.SetValue(HHMMSSsmart);
  data.SetComment(SSsmart);
}

void
InfoBoxContentTaskAATimeDelta::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!common_stats.ordered_has_targets ||
      !task_stats.task_valid || !task_stats.total.IsAchievable() ||
      !positive(task_stats.total.time_remaining) ||
      !positive(common_stats.aat_time_remaining)) {
    data.SetInvalid();
    return;
  }

  fixed diff = task_stats.total.time_remaining -
    common_stats.aat_time_remaining;

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = abs((int)diff);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  data.UnsafeFormatValue(negative(diff) ? _T("-%s") : _T("%s"), HHMMSSsmart);

  data.SetComment(SSsmart);

  // Set Color (red/blue/black)
  data.SetValueColor(negative(diff) ? 1 :
                   task_stats.total.time_remaining <
                       common_stats.aat_time_remaining + fixed(5) ? 2 : 0);
}

void
InfoBoxContentTaskAADistance::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!common_stats.ordered_has_targets ||
      !task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(data, task_stats.total.planned.get_distance());
}

void
InfoBoxContentTaskAADistanceMax::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!common_stats.ordered_has_targets ||
      !task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(data, task_stats.distance_max);
}

void
InfoBoxContentTaskAADistanceMin::Update(InfoBoxData &data)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!common_stats.ordered_has_targets ||
      !task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(data, task_stats.distance_min);
}

void
InfoBoxContentTaskAASpeed::Update(InfoBoxData &data)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!common_stats.ordered_has_targets ||
      !task_stats.task_valid || !positive(common_stats.aat_speed_remaining)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_remaining));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
InfoBoxContentTaskAASpeedMax::Update(InfoBoxData &data)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!common_stats.ordered_has_targets ||
      !task_stats.task_valid || !positive(common_stats.aat_speed_max)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_max));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
InfoBoxContentTaskAASpeedMin::Update(InfoBoxData &data)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!common_stats.ordered_has_targets ||
      !task_stats.task_valid || !positive(common_stats.aat_speed_min)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_min));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
InfoBoxContentTaskTimeUnderMaxHeight::Update(InfoBoxData &data)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const fixed maxheight = fixed(protected_task_manager->
                                GetOrderedTaskBehaviour().start_max_height);

  if (!task_stats.task_valid || !positive(maxheight)
      || !protected_task_manager
      || !positive(common_stats.TimeUnderStartMaxHeight)) {
    data.SetInvalid();
    return;
  }

  const int dd = (int)(XCSoarInterface::Basic().time -
      common_stats.TimeUnderStartMaxHeight);

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  data.SetValue(HHMMSSsmart);
  data.SetComment(_("Time Below"));
}

void
InfoBoxContentNextETEVMG::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::Basic().ground_speed_available) {
    data.SetInvalid();
    return;
  }

  const fixed d = XCSoarInterface::Calculated().task_stats.
    current_leg.remaining.get_distance();
  const fixed &v = XCSoarInterface::Basic().ground_speed;

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !positive(d) ||
      !positive(v)) {
    data.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = (int)(d/v);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  data.SetValue(HHMMSSsmart);
  data.SetComment(SSsmart);
}

void
InfoBoxContentFinalETEVMG::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::Basic().ground_speed_available) {
    data.SetInvalid();
    return;
  }

  const fixed d = XCSoarInterface::Calculated().task_stats.
    total.remaining.get_distance();
  const fixed &v = XCSoarInterface::Basic().ground_speed;

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !positive(d) ||
      !positive(v)) {
    data.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = (int)(d/v);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  data.SetValue(HHMMSSsmart);
  data.SetComment(SSsmart);
}
