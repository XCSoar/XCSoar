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

#include "InfoBoxes/Content/Task.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Dialogs.h"
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
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f")_T(DEG)_T("T"),
            (double)XCSoarInterface::Calculated().task_stats.current_leg.
            solution_remaining.Vector.Bearing.value_degrees());
  infobox.SetValue(tmp);
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
  TCHAR tmp[32];
  double Value =
      (XCSoarInterface::Calculated().task_stats.current_leg.
       solution_remaining.Vector.Bearing - XCSoarInterface::Basic().
       TrackBearing).as_delta().value_degrees();

#ifndef __MINGW32__
  if (Value > 1)
    _stprintf(tmp, _T("%2.0f°»"), Value);
  else if (Value < -1)
    _stprintf(tmp, _T("«%2.0f°"), -Value);
  else
    _tcscpy(tmp, _T("«»"));
#else
  if (Value > 1)
    _stprintf(tmp, _T("%2.0fÂ°Â»"), Value);
  else if (Value < -1)
    _stprintf(tmp, _T("Â«%2.0fÂ°"), -Value);
  else
    _tcscpy(tmp, _T("Â«Â»"));
#endif

  infobox.SetValue(tmp);
}

void
InfoBoxContentNextWaypoint::Update(InfoBoxWindow &infobox)
{
  const Waypoint* way_point = protected_task_manager.getActiveWaypoint();

  if (!way_point) {
    // Set Title
    infobox.SetTitle(_("Next"));

    infobox.SetInvalid();
    return;
  }

  // Set Title
  TCHAR tmp[32];
  if (XCSoarInterface::SettingsMap().DisplayTextType == DISPLAYFIRSTTHREE) {
    _tcsncpy(tmp, way_point->Name.c_str(), 3);
    tmp[3] = '\0';
  } else if (XCSoarInterface::SettingsMap().DisplayTextType == DISPLAYNUMBER) {
    _stprintf(tmp, _T("%d"), way_point->id);
  } else {
    _tcsncpy(tmp, way_point->Name.c_str(), (sizeof(tmp) / sizeof(TCHAR)) - 1);
    tmp[(sizeof(tmp) / sizeof(TCHAR)) - 1] = '\0';
  }
  infobox.SetTitle(tmp);

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.
      Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  double Value =
      (XCSoarInterface::Calculated().task_stats.current_leg.
       solution_remaining.Vector.Bearing - XCSoarInterface::Basic().
       TrackBearing).as_delta().value_degrees();

#ifndef __MINGW32__
  if (Value > 1)
    _stprintf(tmp, _T("%2.0f°»"), Value);
  else if (Value < -1)
    _stprintf(tmp, _T("«%2.0f°"), -Value);
  else
    _tcscpy(tmp, _T("«»"));
#else
  if (Value > 1)
    _stprintf(tmp, _T("%2.0fÂ°Â»"), Value);
  else if (Value < -1)
    _stprintf(tmp, _T("Â«%2.0fÂ°"), -Value);
  else
    _tcscpy(tmp, _T("Â«Â»"));
#endif

  infobox.SetValue(tmp);

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
  case ibkDown:
    protected_task_manager.incrementActiveTaskPoint(1);
    return true;

  case ibkLeft:
  case ibkUp:
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
  _stprintf(tmp, _T("%1.0f"), (double)gradient);
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
  _stprintf(tmp, _T("%1.0f"), (double)gradient);
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
  _stprintf(tmp, _T("%1.0f"), (double)gradient);
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

  _stprintf(tmp, _T("%d")_T(DEG),
            (int)common_stats.vector_home.Bearing.value_degrees());
  infobox.SetComment(tmp);
}

void
InfoBoxContentOLC::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::SettingsComputer().enable_olc) {
    infobox.SetInvalid();
    return;
  }

  const CommonStats &common_stats = XCSoarInterface::Calculated().common_stats;

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserDistance(common_stats.distance_olc, tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);
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
