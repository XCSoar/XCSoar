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

#include "InfoBoxes/Content/Task.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Engine/Util/Gradient.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Units/Units.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Language/Language.hpp"
#include "Widget/CallbackWidget.hpp"

#include <tchar.h>
#include <stdio.h>

static void
ShowNextWaypointDetails()
{
  if (protected_task_manager == nullptr)
    return;

  const Waypoint *wp = protected_task_manager->GetActiveWaypoint();
  if (wp == nullptr)
    return;

  dlgWaypointDetailsShowModal(*wp);
}

static Widget *
LoadNextWaypointDetailsPanel(unsigned id)
{
  return new CallbackWidget(ShowNextWaypointDetails);
}

#ifdef __clang__
/* gcc gives "redeclaration differs in 'constexpr'" */
constexpr
#endif
const InfoBoxPanel next_waypoint_infobox_panels[] = {
  { N_("Details"), LoadNextWaypointDetailsPanel },
  { nullptr, nullptr }
};

void
UpdateInfoBoxBearing(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
  if (!task_stats.task_valid || !vector_remaining.IsValid() ||
      vector_remaining.distance <= fixed(10)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(vector_remaining.bearing);
  data.SetValueColor(task_stats.inside_oz ? 3 : 0);
}

void
UpdateInfoBoxBearingDiff(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
  if (!basic.track_available || !task_stats.task_valid ||
      !vector_remaining.IsValid() || vector_remaining.distance <= fixed(10)) {
    data.SetInvalid();
    return;
  }

  Angle Value = vector_remaining.bearing - basic.track;
  data.SetValueFromBearingDifference(Value);
  data.SetValueColor(task_stats.inside_oz ? 3 : 0);
}

void
UpdateInfoBoxRadial(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
  if (!task_stats.task_valid || !vector_remaining.IsValid() ||
      vector_remaining.distance <= fixed(10)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(vector_remaining.bearing.Reciprocal());
  data.SetValueColor(task_stats.inside_oz ? 3 : 0);

  data.SetCommentFromDistance(vector_remaining.distance);
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

  data.SetTitle(way_point->name.c_str());

  // Set Comment
  if (way_point->radio_frequency.IsDefined()) {
    const unsigned freq = way_point->radio_frequency.GetKiloHertz();
    data.FormatComment(_T("%u.%03u %s"),
                       freq / 1000, freq % 1000, way_point->comment.c_str());
  }
  else
    data.SetComment(way_point->comment.c_str());

  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GlideResult &solution_remaining =
    task_stats.current_leg.solution_remaining;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
  if (!basic.track_available || !task_stats.task_valid ||
      !vector_remaining.IsValid()) {
    data.SetValueInvalid();
    return;
  }

  // Set Value
  Angle Value = vector_remaining.bearing - basic.track;
  data.SetValueFromBearingDifference(Value);

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
  }

  return false;
}

const InfoBoxPanel *
InfoBoxContentNextWaypoint::GetDialogContent()
{
  return next_waypoint_infobox_panels;
}

void
UpdateInfoBoxNextDistance(InfoBoxData &data)
{
  const Waypoint* way_point = protected_task_manager != NULL
    ? protected_task_manager->GetActiveWaypoint()
    : NULL;

  // Set title
  if (!way_point)
    data.SetTitle(_("WP Dist"));
  else
    data.SetTitle(way_point->name.c_str());

  // use proper non-terminal next task stats

  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
  if (!task_stats.task_valid || !vector_remaining.IsValid()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(vector_remaining.distance);
  data.SetValueColor(task_stats.inside_oz ? 3 : 0);

  if (basic.track_available) {
    Angle bd = vector_remaining.bearing - basic.track;
    data.SetCommentFromBearingDifference(bd);
  } else
    data.SetCommentInvalid();
}

void
UpdateInfoBoxNextDistanceNominal(InfoBoxData &data)
{
  const Waypoint* way_point = protected_task_manager != NULL
    ? protected_task_manager->GetActiveWaypoint()
    : NULL;

  if (!way_point) {
    data.SetInvalid();
    return;
  }

  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  if (!task_stats.task_valid || !basic.location_available) {
      data.SetInvalid();
      return;
  }

  const GeoVector vector(basic.location, way_point->location);

  if (!vector.IsValid()) {
      data.SetInvalid();
      return;
  }

  // Set Value
  data.SetValueFromDistance(vector.distance);
  data.SetValueColor(task_stats.inside_oz ? 3 : 0);
  data.SetComment(vector.bearing);
}

void
UpdateInfoBoxNextETE(InfoBoxData &data)
{
  // use proper non-terminal next task stats

  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.current_leg.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  assert(!negative(task_stats.current_leg.time_remaining_now));

  TCHAR value[32];
  TCHAR comment[32];
  const int dd = (int)task_stats.current_leg.time_remaining_now;
  FormatTimeTwoLines(value, comment, dd);

  data.SetValue(value);
  data.SetComment(comment);
}

void
UpdateInfoBoxNextETA(InfoBoxData &data)
{
  // use proper non-terminal next task stats

  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const BrokenTime &now_local = CommonInterface::Calculated().date_time_local;

  if (!task_stats.task_valid || !task_stats.current_leg.IsAchievable() ||
      !now_local.IsPlausible()) {
    data.SetInvalid();
    return;
  }

  const BrokenTime t = now_local +
    unsigned(task_stats.current_leg.solution_remaining.time_elapsed);

  // Set Value
  data.UnsafeFormatValue(_T("%02u:%02u"), t.hour, t.minute);

  // Set Comment
  data.UnsafeFormatComment(_T("%02u"), t.second);
}

static void
SetValueFromAltDiff(InfoBoxData &data, const TaskStats &task_stats,
                    const GlideResult &solution)
{
  if (!task_stats.task_valid || !solution.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  fixed altitude_difference =
    solution.SelectAltitudeDifference(settings.task.glide);
  data.SetValueFromArrival(altitude_difference);
}

void
UpdateInfoBoxNextAltitudeDiff(InfoBoxData &data)
{
  // pilots want this to be assuming terminal flight to this wp

  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GlideResult &next_solution = task_stats.current_leg.solution_remaining;

  SetValueFromAltDiff(data, task_stats, next_solution);
}

void
UpdateInfoBoxNextMC0AltitudeDiff(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  SetValueFromAltDiff(data, task_stats,
                      task_stats.current_leg.solution_mc0);
}

void
UpdateInfoBoxNextAltitudeRequire(InfoBoxData &data)
{
  // pilots want this to be assuming terminal flight to this wp

  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GlideResult &next_solution = task_stats.current_leg.solution_remaining;
  if (!task_stats.task_valid || !next_solution.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(next_solution.GetRequiredAltitude());
}

void
UpdateInfoBoxNextAltitudeArrival(InfoBoxData &data)
{
  // pilots want this to be assuming terminal flight to this wp

  const MoreData &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GlideResult next_solution = task_stats.current_leg.solution_remaining;
  if (!basic.NavAltitudeAvailable() ||
      !task_stats.task_valid || !next_solution.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(next_solution.GetArrivalAltitude(basic.nav_altitude));
}


void
UpdateInfoBoxNextGR(InfoBoxData &data)
{
  // pilots want this to be assuming terminal flight to this wp, and this
  // is what current_leg gradient does.

  if (!CommonInterface::Calculated().task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  fixed gradient = CommonInterface::Calculated().task_stats.current_leg.gradient;

  if (!positive(gradient)) {
    data.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    data.SetValueFromGlideRatio(gradient);
  } else {
    data.SetInvalid();
  }
}

void
UpdateInfoBoxFinalDistance(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.task_stats;

  if (!task_stats.task_valid ||
      !task_stats.current_leg.vector_remaining.IsValid() ||
      !task_stats.total.remaining.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(task_stats.task_finished
                            ? task_stats.current_leg.vector_remaining.distance
                            : task_stats.total.remaining.GetDistance());
}

void
UpdateInfoBoxFinalETE(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  if (!task_stats.task_valid || !task_stats.total.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  assert(!negative(task_stats.total.time_remaining_now));

  TCHAR value[32];
  TCHAR comment[32];
  const int dd = abs((int)task_stats.total.time_remaining_now);
  FormatTimeTwoLines(value, comment, dd);

  data.SetValue(value);
  data.SetComment(comment);
}

void
UpdateInfoBoxFinalETA(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const BrokenTime &now_local = CommonInterface::Calculated().date_time_local;

  if (!task_stats.task_valid || !task_stats.total.IsAchievable() ||
      !now_local.IsPlausible()) {
    data.SetInvalid();
    return;
  }

  const BrokenTime t = now_local +
    unsigned(task_stats.total.solution_remaining.time_elapsed);

  // Set Value
  data.UnsafeFormatValue(_T("%02u:%02u"), t.hour, t.minute);

  // Set Comment
  data.UnsafeFormatComment(_T("%02u"), t.second);
}

void
UpdateInfoBoxFinalAltitudeDiff(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  SetValueFromAltDiff(data, task_stats, task_stats.total.solution_remaining);
}

void
UpdateInfoBoxFinalMC0AltitudeDiff(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  SetValueFromAltDiff(data, task_stats,
                      task_stats.total.solution_mc0);
}

void
UpdateInfoBoxFinalAltitudeRequire(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid ||
      !task_stats.total.solution_remaining.IsOk()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(task_stats.total.solution_remaining.GetRequiredAltitude());
}

void
UpdateInfoBoxTaskSpeed(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.total.travelled.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.total.travelled.GetSpeed()));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
UpdateInfoBoxTaskSpeedAchieved(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid ||
      !task_stats.total.remaining_effective.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.total.remaining_effective.GetSpeed()));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
UpdateInfoBoxTaskSpeedInstant(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.IsPirkerSpeedAvailable()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.0f"),
                    Units::ToUserTaskSpeed(task_stats.get_pirker_speed()));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
UpdateInfoBoxTaskSpeedHour(InfoBoxData &data)
{
  const WindowStats &window =
    CommonInterface::Calculated().task_stats.last_hour;
  if (negative(window.duration)) {
    data.SetInvalid();
    return;
  }

  data.SetValue(_T("%2.0f"), Units::ToUserTaskSpeed(window.speed));
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
UpdateInfoBoxFinalGR(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  fixed gradient = task_stats.total.gradient;

  if (!positive(gradient)) {
    data.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient))
    data.SetValueFromGlideRatio(gradient);
  else
    data.SetInvalid();
}

void
UpdateInfoBoxTaskAATime(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets ||
      !task_stats.total.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  TCHAR value[32];
  TCHAR comment[32];
  FormatTimeTwoLines(value, comment,
                         abs((int) common_stats.aat_time_remaining));

  data.UnsafeFormatValue(negative(common_stats.aat_time_remaining) ?
                            _T("-%s") : _T("%s"), value);
  data.SetValueColor(negative(common_stats.aat_time_remaining) ? 1 : 0);

  data.SetComment(comment);
}

void
UpdateInfoBoxTaskAATimeDelta(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets ||
      !task_stats.total.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  assert(!negative(task_stats.total.time_remaining_start));

  fixed diff = task_stats.total.time_remaining_start -
    common_stats.aat_time_remaining;

  TCHAR value[32];
  TCHAR comment[32];
  const int dd = abs((int)diff);
  FormatTimeTwoLines(value, comment, dd);

  data.UnsafeFormatValue(negative(diff) ? _T("-%s") : _T("%s"), value);

  data.SetComment(comment);

  // Set Color (red/blue/black)
  data.SetValueColor(negative(diff) ? 1 :
                   task_stats.total.time_remaining_start >
                       common_stats.aat_time_remaining + fixed(5*60) ? 2 : 0);
}

void
UpdateInfoBoxTaskAADistance(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;

  if (!task_stats.has_targets ||
      !task_stats.total.planned.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(task_stats.total.planned.GetDistance());
}

void
UpdateInfoBoxTaskAADistanceMax(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;

  if (!task_stats.has_targets) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(task_stats.distance_max);
}

void
UpdateInfoBoxTaskAADistanceMin(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;

  if (!task_stats.has_targets) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(task_stats.distance_min);
}

void
UpdateInfoBoxTaskAASpeed(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets || !positive(common_stats.aat_speed_remaining)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_remaining));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
UpdateInfoBoxTaskAASpeedMax(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets || !positive(common_stats.aat_speed_max)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_max));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
UpdateInfoBoxTaskAASpeedMin(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets ||
      !task_stats.task_valid || !positive(common_stats.aat_speed_min)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.0f"),
                    Units::ToUserTaskSpeed(common_stats.aat_speed_min));

  // Set Unit
  data.SetValueUnit(Units::current.task_speed_unit);
}

void
UpdateInfoBoxTaskTimeUnderMaxHeight(InfoBoxData &data)
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;
  const fixed maxheight = fixed(protected_task_manager->
                                GetOrderedTaskSettings().start_constraints.max_height);

  if (!task_stats.task_valid || !positive(maxheight)
      || !protected_task_manager
      || !positive(common_stats.TimeUnderStartMaxHeight)) {
    data.SetInvalid();
    return;
  }

  const int dd = (int)(CommonInterface::Basic().time -
      common_stats.TimeUnderStartMaxHeight);

  TCHAR value[32];
  TCHAR comment[32];
  FormatTimeTwoLines(value, comment, dd);

  data.SetValue(value);
  data.SetComment(_("Time Below"));
}

void
UpdateInfoBoxNextETEVMG(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  if (!basic.ground_speed_available || !task_stats.task_valid ||
      !task_stats.current_leg.remaining.IsDefined()) {
    data.SetInvalid();
    return;
  }

  const fixed d = task_stats.current_leg.remaining.GetDistance();
  const fixed v = basic.ground_speed;

  if (!task_stats.task_valid ||
      !positive(d) ||
      !positive(v)) {
    data.SetInvalid();
    return;
  }

  TCHAR value[32];
  TCHAR comment[32];
  const int dd = (int)(d/v);
  FormatTimeTwoLines(value, comment, dd);

  data.SetValue(value);
  data.SetComment(comment);
}

void
UpdateInfoBoxFinalETEVMG(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  if (!basic.ground_speed_available || !task_stats.task_valid ||
      !task_stats.total.remaining.IsDefined()) {
    data.SetInvalid();
    return;
  }

  const fixed d = task_stats.total.remaining.GetDistance();
  const fixed v = basic.ground_speed;

  if (!task_stats.task_valid ||
      !positive(d) ||
      !positive(v)) {
    data.SetInvalid();
    return;
  }

  TCHAR value[32];
  TCHAR comment[32];
  const int dd = (int)(d/v);
  FormatTimeTwoLines(value, comment, dd);

  data.SetValue(value);
  data.SetComment(comment);
}

void
UpdateInfoBoxCruiseEfficiency(InfoBoxData &data)
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.start.task_started) {
    data.SetInvalid();
    return;
  }

  data.UnsafeFormatValue(_T("%d"), (int) (task_stats.cruise_efficiency * 100));
  data.SetCommentFromVerticalSpeed(task_stats.effective_mc, false);
}

gcc_pure
static unsigned
SecondsUntil(unsigned now, RoughTime until)
{
  int d = until.GetMinuteOfDay() * 60 - now;
  if (d < 0)
    d += 24 * 60 * 60;
  return d;
}

void
UpdateInfoBoxStartOpen(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  const RoughTimeSpan &open = common_stats.start_open_time_span;

  /* reset color that may have been set by a previous call */
  data.SetValueColor(0);

  if (!basic.time_available || !task_stats.task_valid ||
      common_stats.ordered_summary.active != 0 ||
      !open.IsDefined()) {
    data.SetInvalid();
    return;
  }

  const unsigned now_s(basic.time);
  const RoughTime now = RoughTime::FromSecondOfDayChecked(now_s);

  if (open.HasEnded(now)) {
    data.SetValueInvalid();
    data.SetComment(_("Closed"));
  } else if (open.HasBegun(now)) {
    if (open.GetEnd().IsValid()) {
      unsigned seconds = SecondsUntil(now_s, open.GetEnd());
      data.UnsafeFormatValue(_T("%02u:%02u"), seconds / 60, seconds % 60);
      data.SetValueColor(3);
    } else
      data.SetValueInvalid();

    data.SetComment(_("Open"));
  } else {
    unsigned seconds = SecondsUntil(now_s, open.GetStart());
    data.UnsafeFormatValue(_T("%02u:%02u"), seconds / 60, seconds % 60);
    data.SetValueColor(2);
    data.SetComment(_("Waiting"));
  }
}

void
UpdateInfoBoxStartOpenArrival(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const GlideResult &current_remaining =
    task_stats.current_leg.solution_remaining;
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  const RoughTimeSpan &open = common_stats.start_open_time_span;

  /* reset color that may have been set by a previous call */
  data.SetValueColor(0);

  if (!basic.time_available || !task_stats.task_valid ||
      common_stats.ordered_summary.active != 0 ||
      !open.IsDefined() ||
      !current_remaining.IsOk()) {
    data.SetInvalid();
    return;
  }

  const unsigned arrival_s(basic.time + current_remaining.time_elapsed);
  const RoughTime arrival = RoughTime::FromSecondOfDayChecked(arrival_s);

  if (open.HasEnded(arrival)) {
    data.SetValueInvalid();
    data.SetComment(_("Closed"));
  } else if (open.HasBegun(arrival)) {
    if (open.GetEnd().IsValid()) {
      unsigned seconds = SecondsUntil(arrival_s, open.GetEnd());
      data.UnsafeFormatValue(_T("%02u:%02u"), seconds / 60, seconds % 60);
      data.SetValueColor(3);
    } else
      data.SetValueInvalid();

    data.SetComment(_("Open"));
  } else {
    unsigned seconds = SecondsUntil(arrival_s, open.GetStart());
    data.UnsafeFormatValue(_T("%02u:%02u"), seconds / 60, seconds % 60);
    data.SetValueColor(2);
    data.SetComment(_("Waiting"));
  }
}
