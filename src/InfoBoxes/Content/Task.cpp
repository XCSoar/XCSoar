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

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Dialogs/Dialogs.h"
#include "MainWindow.hpp"
#include "LocalTime.hpp"
#include "Engine/Util/Gradient.hpp"
#include "UnitsFormatter.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentBearing::Update(InfoBoxWindow &infobox)
{
  const GlideResult &solution_remaining =
    XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining;
  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !solution_remaining.defined() ||
      solution_remaining.Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  infobox.SetValue(solution_remaining.Vector.Bearing, _T("T"));
}

void
InfoBoxContentBearingDiff::Update(InfoBoxWindow &infobox)
{
  const GlideResult &solution_remaining =
    XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining;
  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !solution_remaining.defined() ||
      solution_remaining. Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Angle Value =
    solution_remaining.Vector.Bearing - XCSoarInterface::Basic().TrackBearing;

  SetValueBearingDifference(infobox, Value);
}

void
InfoBoxContentNextWaypoint::Update(InfoBoxWindow &infobox)
{
  // use proper non-terminal next task stats

  const Waypoint* way_point = protected_task_manager != NULL
    ? protected_task_manager->getActiveWaypoint()
    : NULL;

  if (!way_point) {
    infobox.SetTitle(_("Next"));
    infobox.SetInvalid();
    return;
  }
  SetTitleFromWaypointName(infobox, way_point);

  // Set Comment
  infobox.SetComment(way_point->Comment.c_str());

  const GlideResult &solution_remaining =
    XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining;
  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !solution_remaining.defined() ||
      solution_remaining.Vector.Distance <= fixed(10)) {
    infobox.SetValueInvalid();
    return;
  }

  // Set Value
  Angle Value =
    solution_remaining.Vector.Bearing - XCSoarInterface::Basic().TrackBearing;

  SetValueBearingDifference(infobox, Value);

  // Set Color (blue/black)
  infobox.SetColor(solution_remaining.is_final_glide() ? 2 : 0);
}

bool
InfoBoxContentNextWaypoint::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (protected_task_manager == NULL)
    return false;

  switch (keycode) {
  case ibkRight:
  case ibkUp:
    protected_task_manager->incrementActiveTaskPoint(1);
    return true;

  case ibkLeft:
  case ibkDown:
    protected_task_manager->incrementActiveTaskPoint(-1);
    return true;

  case ibkEnter:
    const Waypoint *wp = protected_task_manager->getActiveWaypoint();
    if (wp) {
      dlgWayPointDetailsShowModal(XCSoarInterface::main_window, *wp);
      return true;
    }
  }

  return false;
}

void
InfoBoxContentNextDistance::Update(InfoBoxWindow &infobox)
{
  // use proper non-terminal next task stats

  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const GlideResult &solution_remaining =
    XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining;
  if (!task_stats.task_valid || !solution_remaining.defined()) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(infobox, solution_remaining.Vector.Distance);
}

void
InfoBoxContentNextETE::Update(InfoBoxWindow &infobox)
{
  // use proper non-terminal next task stats

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !XCSoarInterface::Calculated().task_stats.current_leg.achievable() ||
      !positive(XCSoarInterface::Calculated().task_stats.
                current_leg.TimeRemaining)) {
    infobox.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = (int)XCSoarInterface::Calculated().
      task_stats.current_leg.TimeRemaining;
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  infobox.SetValue(HHMMSSsmart);
  infobox.SetComment(SSsmart);
}

void
InfoBoxContentNextETA::Update(InfoBoxWindow &infobox)
{
  // use proper non-terminal next task stats

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !XCSoarInterface::Calculated().task_stats.current_leg.achievable()) {
    infobox.SetInvalid();
    return;
  }

  TCHAR tmp[32];
  int dd = abs((int)(XCSoarInterface::Calculated().task_stats.current_leg.
      solution_remaining.TimeElapsed +
      fixed(DetectCurrentTime(&XCSoarInterface::Basic())))) % (3600 * 24);
  int hours = (dd / 3600);
  int mins = (dd / 60 - hours * 60);
  int seconds = (dd - mins * 60 - hours * 3600);
  hours = hours % 24;

  // Set Value
  _stprintf(tmp, _T("%02d:%02d"), hours, mins);
  infobox.SetValue(tmp);

  // Set Comment
  _stprintf(tmp, _T("%02d"), seconds);
  infobox.SetComment(tmp);
}

void
InfoBoxContentNextAltitudeDiff::Update(InfoBoxWindow &infobox)
{
  // pilots want this to be assuming terminal flight to this wp

  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const GlideResult &next_solution = XCSoarInterface::Calculated().common_stats.next_solution;
  if (!task_stats.task_valid || !next_solution.defined()) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(next_solution.AltitudeDifference, tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}

void
InfoBoxContentNextAltitudeRequire::Update(InfoBoxWindow &infobox)
{
  // pilots want this to be assuming terminal flight to this wp

  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const GlideResult &next_solution = XCSoarInterface::Calculated().common_stats.next_solution;
  if (!task_stats.task_valid || !next_solution.defined()) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(next_solution.AltitudeRequired, tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}

void
InfoBoxContentNextAltitudeArrival::Update(InfoBoxWindow &infobox)
{
  // pilots want this to be assuming terminal flight to this wp

  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const GlideResult next_solution = XCSoarInterface::Calculated().common_stats.next_solution;
  if (!task_stats.task_valid || !next_solution.glide_reachable(true)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  fixed alt = XCSoarInterface::Basic().NavAltitude-next_solution.HeightGlide;
  Units::FormatUserAltitude(alt, tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}


void
InfoBoxContentNextLD::Update(InfoBoxWindow &infobox)
{
  // pilots want this to be assuming terminal flight to this wp, and this
  // is what current_leg gradient does.

  if (!XCSoarInterface::Calculated().task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  fixed gradient = XCSoarInterface::Calculated().task_stats.current_leg.gradient;

  if (!positive(gradient)) {
    infobox.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    TCHAR tmp[32];
    _stprintf(tmp, _T("%d"), (int)gradient);
    infobox.SetValue(tmp);
  } else {
    infobox.SetInvalid();
  }
}

void
InfoBoxContentFinalDistance::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!task_stats.task_valid ||
      !task_stats.current_leg.solution_remaining.defined()) {
    infobox.SetInvalid();
    return;
  }

  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  // Set Value
  SetValueFromDistance(infobox, common_stats.task_finished ?
                                task_stats.current_leg.solution_remaining.Vector.Distance :
                                task_stats.total.remaining.get_distance());
}

void
InfoBoxContentFinalETE::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!task_stats.task_valid || !task_stats.total.achievable() ||
      !positive(task_stats.total.TimeRemaining)) {
    infobox.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = abs((int)task_stats.total.TimeRemaining);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  infobox.SetValue(HHMMSSsmart);
  infobox.SetComment(SSsmart);
}

void
InfoBoxContentFinalETA::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.total.achievable()) {
    infobox.SetInvalid();
    return;
  }

  TCHAR tmp[32];
  int dd = abs((int)(task_stats.total.solution_remaining.TimeElapsed +
      fixed(DetectCurrentTime(&XCSoarInterface::Basic())))) % (3600 * 24);
  int hours = (dd / 3600);
  int mins = (dd / 60 - hours * 60);
  int seconds = (dd - mins * 60 - hours * 3600);
  hours = hours % 24;

  // Set Value
  _stprintf(tmp, _T("%02d:%02d"), hours, mins);
  infobox.SetValue(tmp);

  // Set Comment
  _stprintf(tmp, _T("%02d"), seconds);
  infobox.SetComment(tmp);
}

void
InfoBoxContentFinalAltitudeDiff::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid ||
      !task_stats.total.solution_remaining.defined()) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(task_stats.total.solution_remaining.AltitudeDifference,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}

void
InfoBoxContentFinalAltitudeRequire::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid ||
      !task_stats.total.solution_remaining.defined()) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(task_stats.total.solution_remaining.AltitudeRequired,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}

void
InfoBoxContentTaskSpeed::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.total.travelled.get_speed()));

  // Set Unit
  infobox.SetValueUnit(Units::Current.TaskSpeedUnit);
}

void
InfoBoxContentTaskSpeedAchieved::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.total.remaining_effective.get_speed()));

  // Set Unit
  infobox.SetValueUnit(Units::Current.TaskSpeedUnit);
}

void
InfoBoxContentTaskSpeedInstant::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.get_pirker_speed()));

  // Set Unit
  infobox.SetValueUnit(Units::Current.TaskSpeedUnit);
}

void
InfoBoxContentFinalLD::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  fixed gradient = task_stats.total.gradient;

  if (!positive(gradient)) {
    infobox.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    TCHAR tmp[32];
    _stprintf(tmp, _T("%d"), (int)gradient);
    infobox.SetValue(tmp);
  } else {
    infobox.SetInvalid();
  }
}

void
InfoBoxContentFinalGR::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  fixed gradient = task_stats.total.gradient;

  if (!positive(gradient)) {
    infobox.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    TCHAR tmp[32];
    _stprintf(tmp, _T("%d"), (int)gradient);
    infobox.SetValue(tmp);
  } else {
    infobox.SetInvalid();
  }
}

void
InfoBoxContentHomeDistance::Update(InfoBoxWindow &infobox)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  // Set Value
  SetValueFromDistance(infobox, common_stats.vector_home.Distance);

  infobox.SetComment(common_stats.vector_home.Bearing);
}

void
InfoBoxContentOLC::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::SettingsComputer().enable_olc ||
      !protected_task_manager) {
    infobox.SetInvalid();
    return;
  }

  const ContestResult& result_olc = 
    XCSoarInterface::Calculated().contest_stats.get_contest_result();

  if (result_olc.score < fixed_one) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(infobox, result_olc.distance);

  TCHAR tmp[32];
  _stprintf(tmp, _T("%.1f pts"), (double)result_olc.score);
  infobox.SetComment(tmp);
}

bool
InfoBoxContentOLC::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    dlgAnalysisShowModal(XCSoarInterface::main_window, 8);
    return true;

  default:
    break;
  }

  return false;
}

void
InfoBoxContentTaskAATime::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!task_stats.task_valid || !task_stats.total.achievable() ||
      !positive(common_stats.aat_time_remaining)) {
    infobox.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = abs((int)common_stats.aat_time_remaining);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  infobox.SetValue(HHMMSSsmart);
  infobox.SetComment(SSsmart);
}

void
InfoBoxContentTaskAATimeDelta::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!task_stats.task_valid || !task_stats.total.achievable() ||
      !positive(task_stats.total.TimeRemaining) ||
      !positive(common_stats.aat_time_remaining)) {
    infobox.SetInvalid();
    return;
  }

  fixed diff = task_stats.total.TimeRemaining -
    common_stats.aat_time_remaining;

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = abs((int)diff);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  TCHAR tmp[32];
  _stprintf(tmp, negative(diff) ? _T("-%s") : _T("%s"), HHMMSSsmart);
  infobox.SetValue(tmp);

  infobox.SetComment(SSsmart);

  // Set Color (red/blue/black)
  infobox.SetColor(negative(diff) ? 1 :
                   task_stats.total.TimeRemaining <
                       common_stats.aat_time_remaining + fixed(5) ? 2 : 0);
}

void
InfoBoxContentTaskAADistance::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(infobox, task_stats.total.planned.get_distance());
}

void
InfoBoxContentTaskAADistanceMax::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(infobox, task_stats.distance_max);
}

void
InfoBoxContentTaskAADistanceMin::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromDistance(infobox, task_stats.distance_min);
}

void
InfoBoxContentTaskAASpeed::Update(InfoBoxWindow &infobox)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!task_stats.task_valid || !positive(common_stats.aat_speed_remaining)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_remaining));

  // Set Unit
  infobox.SetValueUnit(Units::Current.TaskSpeedUnit);
}

void
InfoBoxContentTaskAASpeedMax::Update(InfoBoxWindow &infobox)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!task_stats.task_valid || !positive(common_stats.aat_speed_max)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_max));

  // Set Unit
  infobox.SetValueUnit(Units::Current.TaskSpeedUnit);
}

void
InfoBoxContentTaskAASpeedMin::Update(InfoBoxWindow &infobox)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  if (!task_stats.task_valid || !positive(common_stats.aat_speed_min)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_min));

  // Set Unit
  infobox.SetValueUnit(Units::Current.TaskSpeedUnit);
}

void
InfoBoxContentTaskTimeUnderMaxHeight::Update(InfoBoxWindow &infobox)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  const fixed maxheight = fixed(protected_task_manager->
                                get_ordered_task_behaviour().start_max_height);

  if (!task_stats.task_valid || !positive(maxheight)
      || !protected_task_manager
      || !positive(common_stats.TimeUnderStartMaxHeight)) {
    infobox.SetInvalid();
    return;
  }

  const int dd = (int)(XCSoarInterface::Basic().Time -
      common_stats.TimeUnderStartMaxHeight);

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  infobox.SetValue(HHMMSSsmart);
  infobox.SetComment(_("Time Below"));
}

void
InfoBoxContentNextETEVMG::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Basic().GroundSpeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  const fixed d = XCSoarInterface::Calculated().task_stats.
    current_leg.remaining.get_distance();
  const fixed &v = XCSoarInterface::Basic().GroundSpeed;

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !positive(d) ||
      !positive(v)) {
    infobox.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = (int)(d/v);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  infobox.SetValue(HHMMSSsmart);
  infobox.SetComment(SSsmart);
}

void
InfoBoxContentFinalETEVMG::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Basic().GroundSpeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  const fixed d = XCSoarInterface::Calculated().task_stats.
    total.remaining.get_distance();
  const fixed &v = XCSoarInterface::Basic().GroundSpeed;

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !positive(d) ||
      !positive(v)) {
    infobox.SetInvalid();
    return;
  }

  TCHAR HHMMSSsmart[32];
  TCHAR SSsmart[32];
  const int dd = (int)(d/v);
  Units::TimeToTextSmart(HHMMSSsmart, SSsmart, dd);

  infobox.SetValue(HHMMSSsmart);
  infobox.SetComment(SSsmart);
}
