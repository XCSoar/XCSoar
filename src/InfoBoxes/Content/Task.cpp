/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentBearing::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.
      Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  infobox.SetValue(XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.Vector.Bearing,
                   _T("T"));
}

void
InfoBoxContentBearingDiff::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.
      Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Angle Value =
      (XCSoarInterface::Calculated().task_stats.current_leg.
       solution_remaining.Vector.Bearing - XCSoarInterface::Basic().
       TrackBearing);

  SetValueBearingDifference(infobox, Value);
}

void
InfoBoxContentNextWaypoint::Update(InfoBoxWindow &infobox)
{
  const Waypoint* way_point = protected_task_manager.getActiveWaypoint();

  if (!way_point) {
    infobox.SetTitle(_("Next"));
    infobox.SetInvalid();
    return;
  }
  SetTitleFromWaypointName(infobox, way_point);

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.
      Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Angle Value =
      (XCSoarInterface::Calculated().task_stats.current_leg.
       solution_remaining.Vector.Bearing - XCSoarInterface::Basic().
       TrackBearing);

  SetValueBearingDifference(infobox, Value);

  // Set Comment
  infobox.SetComment(way_point->Comment.c_str());

  // Set Color
  if (XCSoarInterface::Calculated().task_stats.current_leg.
      solution_remaining.is_final_glide())
    // blue
    infobox.SetColor(2);
  else
    // black
    infobox.SetColor(0);
}

bool
InfoBoxContentNextWaypoint::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkRight:
  case ibkUp:
    protected_task_manager.incrementActiveTaskPoint(1);
    return true;

  case ibkLeft:
  case ibkDown:
    protected_task_manager.incrementActiveTaskPoint(-1);
    return true;

  case ibkEnter:
    const Waypoint *wp = protected_task_manager.getActiveWaypoint();
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
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserDistance(task_stats.current_leg.solution_remaining.Vector.Distance,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);
}

void
InfoBoxContentNextETE::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      !XCSoarInterface::Calculated().task_stats.current_leg.achievable() ||
      !positive(XCSoarInterface::Calculated().task_stats.
                current_leg.TimeRemaining)) {
    infobox.SetInvalid();
    return;
  }

  TCHAR tmp[32];
  int dd = abs((int)XCSoarInterface::Calculated().
               task_stats.current_leg.TimeRemaining) % (3600 * 24);
  int hours = (dd / 3600);
  int mins = (dd / 60 - hours * 60);
  int seconds = (dd - mins * 60 - hours * 3600);
  hours = hours % 24;

  if (hours > 0) { // hh:mm, ss
    // Set Value
    _stprintf(tmp, _T("%02d:%02d"), hours, mins);
    infobox.SetValue(tmp);

    // Set Comment
    _stprintf(tmp, _T("%02d"), seconds);
    infobox.SetComment(tmp);
  } else { // mm:ss
    // Set Value
    _stprintf(tmp, _T("%02d:%02d"), mins, seconds);
    infobox.SetValue(tmp);

    // Set Comment
    infobox.SetComment(_T(""));
  }
}

void
InfoBoxContentNextETA::Update(InfoBoxWindow &infobox)
{
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
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(task_stats.current_leg.solution_remaining.AltitudeDifference,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentNextAltitudeRequire::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(task_stats.current_leg.solution_remaining.AltitudeRequired,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentNextLD::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  fixed gradient = XCSoarInterface::Calculated().task_stats.current_leg.gradient;

  if (!positive(gradient) || gradient > fixed(500)) {
    infobox.SetValue(_T("+++"));
    return;
  }

  TCHAR tmp[32];
  _stprintf(tmp, _T("%d"), (int)gradient);
  infobox.SetValue(tmp);
}

void
InfoBoxContentFinalDistance::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserDistance(common_stats.task_finished
                            ? task_stats.current_leg.solution_remaining.Vector.Distance
                            : task_stats.total.remaining.get_distance(),
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);
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

  TCHAR tmp[32];
  int dd = abs((int)task_stats.total.TimeRemaining) % (3600 * 24);
  int hours = (dd / 3600);
  int mins = (dd / 60 - hours * 60);
  int seconds = (dd - mins * 60 - hours * 3600);
  hours = hours % 24;

  if (hours > 0) { // hh:mm, ss
    // Set Value
    _stprintf(tmp, _T("%02d:%02d"), hours, mins);
    infobox.SetValue(tmp);

    // Set Comment
    _stprintf(tmp, _T("%02d"), seconds);
    infobox.SetComment(tmp);
  } else { // mm:ss
    // Set Value
    _stprintf(tmp, _T("%02d:%02d"), mins, seconds);
    infobox.SetValue(tmp);

    // Set Comment
    infobox.SetComment(_T(""));
  }
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
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(task_stats.total.solution_remaining.AltitudeDifference,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentFinalAltitudeRequire::Update(InfoBoxWindow &infobox)
{
  const TaskStats &task_stats = XCSoarInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserAltitude(task_stats.total.solution_remaining.AltitudeRequired,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
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
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"),
            (double)Units::ToUserTaskSpeed(task_stats.total.travelled.get_speed()));
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::TaskSpeedUnit);
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
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"),
            (double)Units::ToUserTaskSpeed(task_stats.total.travelled.get_speed()));
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::TaskSpeedUnit);
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
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"),
            (double)Units::ToUserTaskSpeed(task_stats.total.remaining_effective.
                                           get_speed_incremental()));
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::TaskSpeedUnit);
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

  if (!positive(gradient) || gradient > fixed(500)) {
    infobox.SetValue(_T("+++"));
    return;
  }

  TCHAR tmp[32];
  _stprintf(tmp, _T("%d"), (int)gradient);
  infobox.SetValue(tmp);
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

  if (!positive(gradient) || gradient > fixed(500)) {
    infobox.SetValue(_T("+++"));
    return;
  }

  TCHAR tmp[32];
  _stprintf(tmp, _T("%d"), (int)gradient);
  infobox.SetValue(tmp);
}

void
InfoBoxContentHomeDistance::Update(InfoBoxWindow &infobox)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserDistance(common_stats.vector_home.Distance, tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);

  infobox.SetComment(common_stats.vector_home.Bearing);
}

void
InfoBoxContentOLC::Update(InfoBoxWindow &infobox)
{
  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  if (!XCSoarInterface::SettingsComputer().enable_olc ||
      common_stats.olc.score < fixed_one) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserDistance(common_stats.olc.distance, tmp, 32, false);
  infobox.SetValue(tmp);

  _stprintf(tmp, _T("%.1f pts"), (double)common_stats.olc.score);
  infobox.SetComment(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);
}

bool
InfoBoxContentOLC::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    dlgAnalysisShowModal(XCSoarInterface::main_window, 7);
    return true;

  case ibkUp:
  case ibkDown:
  case ibkLeft:
  case ibkRight:
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

  TCHAR tmp[32];
  int dd = abs((int)common_stats.aat_time_remaining) % (3600 * 24);
  int hours = (dd / 3600);
  int mins = (dd / 60 - hours * 60);
  int seconds = (dd - mins * 60 - hours * 3600);
  hours = hours % 24;

  if (hours > 0) { // hh:mm, ss
    // Set Value
    _stprintf(tmp, _T("%02d:%02d"), hours, mins);
    infobox.SetValue(tmp);

    // Set Comment
    _stprintf(tmp, _T("%02d"), seconds);
    infobox.SetComment(tmp);
  } else { // mm:ss
    // Set Value
    _stprintf(tmp, _T("%02d:%02d"), mins, seconds);
    infobox.SetValue(tmp);

    // Set Comment
    infobox.SetComment(_T(""));
  }
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

  TCHAR tmp[32];
  fixed diff = task_stats.total.TimeRemaining -
    common_stats.aat_time_remaining;
  int dd = abs((int)diff) % (3600 * 24);
  int hours = (dd / 3600);
  int mins = (dd / 60 - hours * 60);
  int seconds = (dd - mins * 60 - hours * 3600);
  hours = hours % 24;

  if (hours > 0) { // hh:mm, ss
    // Set Value
    _stprintf(tmp, negative(diff) ? _T("-%02d:%02d") : _T("%02d:%02d"),
              hours, mins);
    infobox.SetValue(tmp);

    // Set Comment
    _stprintf(tmp, _T("%02d"), seconds);
    infobox.SetComment(tmp);
  } else { // mm:ss
    // Set Value
    _stprintf(tmp, negative(diff) ? _T("-%02d:%02d") : _T("%02d:%02d"),
              mins, seconds);
    infobox.SetValue(tmp);

    // Set Comment
    infobox.SetComment(_T(""));
  }

  // Set Color
  if (negative(diff))
    // Red
    infobox.SetColor(1);
  else if (task_stats.total.TimeRemaining <
           common_stats.aat_time_remaining + fixed(5))
    // Blue
    infobox.SetColor(2);
  else
    // Black
    infobox.SetColor(0);
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
  TCHAR tmp[32];
  Units::FormatUserDistance(task_stats.total.planned.get_distance(),
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);
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
  TCHAR tmp[32];
  Units::FormatUserDistance(task_stats.distance_max,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);
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
  TCHAR tmp[32];
  Units::FormatUserDistance(task_stats.distance_min,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);
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
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"), (double)Units::ToUserTaskSpeed(
      common_stats.aat_speed_remaining));
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::TaskSpeedUnit);
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
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"), (double)Units::ToUserTaskSpeed(
      common_stats.aat_speed_max));
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::TaskSpeedUnit);
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
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"), (double)Units::ToUserTaskSpeed(
      common_stats.aat_speed_min));
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::TaskSpeedUnit);
}
