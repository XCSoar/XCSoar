// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"
#include "Widget/CallbackWidget.hpp"
#include "Renderer/NextArrowRenderer.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"

#include <tchar.h>

static void
ShowNextWaypointDetails() noexcept
{
  if (!backend_components->protected_task_manager)
    return;

  auto wp = backend_components->protected_task_manager->GetActiveWaypoint();
  if (wp == nullptr)
    return;

  dlgWaypointDetailsShowModal(data_components->waypoints.get(),
                              std::move(wp), false);
}

static std::unique_ptr<Widget>
LoadNextWaypointDetailsPanel([[maybe_unused]] unsigned id) noexcept
{
  return std::make_unique<CallbackWidget>(ShowNextWaypointDetails);
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
UpdateInfoBoxBearing(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
  if (!task_stats.task_valid || !vector_remaining.IsValid() ||
      vector_remaining.distance <= 10) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(vector_remaining.bearing);
  data.SetValueColor(task_stats.inside_oz ? 3 : 5);
}

void
UpdateInfoBoxBearingDiff(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
  if (!basic.track_available || !task_stats.task_valid ||
      !vector_remaining.IsValid() || vector_remaining.distance <= 10) {
    data.SetInvalid();
    return;
  }

  Angle Value = vector_remaining.bearing - basic.track;
  data.SetValueFromBearingDifference(Value);
  data.SetValueColor(task_stats.inside_oz ? 3 : 0);
}

void
UpdateInfoBoxRadial(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
  if (!task_stats.task_valid || !vector_remaining.IsValid() ||
      vector_remaining.distance <= 10) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(vector_remaining.bearing.Reciprocal());
  data.SetValueColor(task_stats.inside_oz ? 3 : 5);

  data.SetCommentFromDistance(vector_remaining.distance);
}

void
InfoBoxContentNextWaypoint::Update(InfoBoxData &data) noexcept
{
  // use proper non-terminal next task stats

  const auto way_point = backend_components->protected_task_manager
    ? backend_components->protected_task_manager->GetActiveWaypoint()
    : nullptr;

  if (!way_point) {
    data.SetTitle(_("Next"));
    data.SetInvalid();
    return;
  }

  data.SetTitle(way_point->name.c_str());

  // Set Comment
  if (way_point->radio_frequency.IsDefined()) {
    const unsigned freq = way_point->radio_frequency.GetKiloHertz();
    data.FmtComment(_T("{}.{:03} {}"),
                    freq / 1000, freq % 1000, way_point->comment);
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
  data.SetValueColor(solution_remaining.IsFinalGlide() ? 2 : 5);
}

const InfoBoxPanel *
InfoBoxContentNextWaypoint::GetDialogContent() noexcept
{
  return next_waypoint_infobox_panels;
}

void
UpdateInfoBoxNextDistance(InfoBoxData &data) noexcept
{
  const auto way_point = backend_components->protected_task_manager
    ? backend_components->protected_task_manager->GetActiveWaypoint()
    : nullptr;

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
  data.SetValueColor(task_stats.inside_oz ? 3 : 5);

  if (basic.track_available) {
    Angle bd = vector_remaining.bearing - basic.track;
    data.SetCommentFromBearingDifference(bd);
  } else
    data.SetCommentInvalid();
}

void
UpdateInfoBoxNextDistanceNominal(InfoBoxData &data) noexcept
{
  const auto way_point = backend_components->protected_task_manager
    ? backend_components->protected_task_manager->GetActiveWaypoint()
    : nullptr;

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
  data.SetValueColor(task_stats.inside_oz ? 3 : 5);
  data.SetComment(vector.bearing);
}

void
UpdateInfoBoxNextETE(InfoBoxData &data) noexcept
{
  // use proper non-terminal next task stats

  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.current_leg.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  assert(task_stats.current_leg.time_remaining_now.count() >= 0);

  data.SetValueFromTimeTwoLines(task_stats.current_leg.time_remaining_now);
  data.SetValueColor(5);
}

void
UpdateInfoBoxNextETA(InfoBoxData &data) noexcept
{
  // use proper non-terminal next task stats

  const auto &task_stats = CommonInterface::Calculated().task_stats;
  const BrokenTime &now_local = CommonInterface::Calculated().date_time_local;

  if (!task_stats.task_valid || !task_stats.current_leg.IsAchievable() ||
      !now_local.IsPlausible()) {
    data.SetInvalid();
    return;
  }

  const BrokenTime t = now_local +
    std::chrono::duration_cast<std::chrono::seconds>(task_stats.current_leg.solution_remaining.time_elapsed);

  // Set Value
  data.FmtValue(_T("{:02}:{:02}"), t.hour, t.minute);

  // Set Comment
  data.FmtComment(_T("{:02}"), t.second);
  
  data.SetValueColor(5);
}

static void
SetValueFromAltDiff(InfoBoxData &data, const TaskStats &task_stats,
                    const GlideResult &solution)
{
  if (!task_stats.task_valid || !solution.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  const auto &settings = CommonInterface::GetComputerSettings();
  auto altitude_difference =
    solution.SelectAltitudeDifference(settings.task.glide);
  data.SetValueFromArrival(altitude_difference);
}

void
UpdateInfoBoxNextAltitudeDiff(InfoBoxData &data) noexcept
{
  // pilots want this to be assuming terminal flight to this wp

  const auto &task_stats = CommonInterface::Calculated().task_stats;
  const auto &next_solution = task_stats.current_leg.solution_remaining;

  SetValueFromAltDiff(data, task_stats, next_solution);
  data.SetValueColor(5);
}

void
UpdateInfoBoxNextMC0AltitudeDiff(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  SetValueFromAltDiff(data, task_stats,
                      task_stats.current_leg.solution_mc0);
  data.SetValueColor(5);
}

void
UpdateInfoBoxNextAltitudeRequire(InfoBoxData &data) noexcept
{
  // pilots want this to be assuming terminal flight to this wp

  const auto &task_stats = CommonInterface::Calculated().task_stats;
  const auto &next_solution = task_stats.current_leg.solution_remaining;
  if (!task_stats.task_valid || !next_solution.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(next_solution.GetRequiredAltitude());
  data.SetValueColor(5);
}

void
UpdateInfoBoxNextAltitudeArrival(InfoBoxData &data) noexcept
{
  // pilots want this to be assuming terminal flight to this wp

  const auto &basic = CommonInterface::Basic();
  const auto &task_stats = CommonInterface::Calculated().task_stats;
  const auto next_solution = task_stats.current_leg.solution_remaining;
  if (!basic.NavAltitudeAvailable() ||
      !task_stats.task_valid || !next_solution.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(next_solution.GetArrivalAltitude(basic.nav_altitude));
  data.SetValueColor(5);
}


void
UpdateInfoBoxNextGR(InfoBoxData &data) noexcept
{
  // pilots want this to be assuming terminal flight to this wp, and this
  // is what current_leg gradient does.
  data.SetValueColor(5);

  if (!CommonInterface::Calculated().task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  auto gradient = CommonInterface::Calculated().task_stats.current_leg.gradient;

  if (gradient <= 0) {
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
UpdateInfoBoxFinalDistance(InfoBoxData &data) noexcept
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
UpdateInfoBoxFinalETE(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  if (!task_stats.task_valid || !task_stats.total.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  assert(task_stats.total.time_remaining_now.count() >= 0);

  data.SetValueFromTimeTwoLines(task_stats.total.time_remaining_now);
}

void
UpdateInfoBoxFinalETA(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const BrokenTime &now_local = CommonInterface::Calculated().date_time_local;

  if (!task_stats.task_valid || !task_stats.total.IsAchievable() ||
      !now_local.IsPlausible()) {
    data.SetInvalid();
    return;
  }

  const BrokenTime t = now_local +
    std::chrono::duration_cast<std::chrono::seconds>(task_stats.total.solution_remaining.time_elapsed);

  // Set Value
  data.FmtValue(_T("{:02}:{:02}"), t.hour, t.minute);

  // Set Comment
  data.FmtComment(_T("{:02}"), t.second);
}

void
UpdateInfoBoxFinalAltitudeDiff(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  SetValueFromAltDiff(data, task_stats, task_stats.total.solution_remaining);
}

void
UpdateInfoBoxFinalMC0AltitudeDiff(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  SetValueFromAltDiff(data, task_stats,
                      task_stats.total.solution_mc0);
}

void
UpdateInfoBoxFinalAltitudeRequire(InfoBoxData &data) noexcept
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
UpdateInfoBoxTaskSpeed(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.total.travelled.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value and unit
  data.SetValueFromTaskSpeed(task_stats.total.travelled.GetSpeed());
}

void
UpdateInfoBoxTaskSpeedAchieved(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid ||
      !task_stats.total.remaining_effective.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value and unit
  data.SetValueFromTaskSpeed(task_stats.total.remaining_effective.GetSpeed());
}

void
UpdateInfoBoxTaskSpeedInstant(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || task_stats.inst_speed_fast < 0 ||
      task_stats.inst_speed_slow < 0) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromTaskSpeed(task_stats.inst_speed_fast);

  // Add slow filtered task speed as comment item
  data.SetCommentFromTaskSpeed(task_stats.inst_speed_slow, false);
}

void
UpdateInfoBoxTaskSpeedHour(InfoBoxData &data) noexcept
{
  const WindowStats &window =
    CommonInterface::Calculated().task_stats.last_hour;
  if (!window.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value and unit
  data.SetValueFromTaskSpeed(window.speed);
}

void
UpdateInfoBoxTaskSpeedEst(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.total.planned.IsDefined() ||
      !task_stats.total.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  // Set Value and unit
  data.SetValueFromTaskSpeed(task_stats.total.planned.GetSpeed());
}

void
UpdateInfoBoxFinalGR(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  auto gradient = task_stats.total.gradient;

  if (gradient <= 0) {
    data.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient))
    data.SetValueFromGlideRatio(gradient);
  else
    data.SetInvalid();
}

void
UpdateInfoBoxTaskAATime(InfoBoxData &data) noexcept
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets ||
      !task_stats.total.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromTimeTwoLines(common_stats.aat_time_remaining);
  data.SetValueColor(common_stats.aat_time_remaining.count() < 0 ? 1 : 0);
}

void
UpdateInfoBoxTaskAATimeDelta(InfoBoxData &data) noexcept
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets ||
      !task_stats.total.IsAchievable()) {
    data.SetInvalid();
    return;
  }

  assert(task_stats.total.time_remaining_start.count() >= 0);

  auto diff = task_stats.total.time_remaining_start -
    common_stats.aat_time_remaining;

  data.SetValueFromTimeTwoLines(diff);
  // Set Color (red/blue/black)
  data.SetValueColor(diff.count() < 0 ? 1 :
                     task_stats.total.time_remaining_start >
                     common_stats.aat_time_remaining + std::chrono::minutes{5} ? 2 : 0);
}

void
UpdateInfoBoxTaskAADistance(InfoBoxData &data) noexcept
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  if (!task_stats.has_targets ||
      !task_stats.total.planned.IsDefined()) {
    data.SetInvalid();
    data.SetCommentInvalid();
    data.SetAllColors(0);
    return;
  }

  // Set Value
  double distance = task_stats.total.planned.GetDistance();
  data.SetValueFromDistance(distance);

  if (map_settings.show_95_percent_rule_helpers) {
    double fractionTotal = distance / task_stats.distance_max_total;
    data.SetCommentFromPercent(fractionTotal*100.0);

    if (fractionTotal > 0.95) data.SetAllColors(3);       // green
    else if (fractionTotal > 0.85) data.SetAllColors(4);  // yellow
    else data.SetAllColors(0);                            // normal
  }
  else {
    data.SetCommentInvalid();
    data.SetAllColors(0);
  }
}

void
UpdateInfoBoxTaskAADistanceMax(InfoBoxData &data) noexcept
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  if (!task_stats.has_targets) {
    data.SetInvalid();
    data.SetCommentInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(task_stats.distance_max);

  if (map_settings.show_95_percent_rule_helpers) {
    auto distance = FormatUserDistanceSmart(0.95*task_stats.distance_max_total);
    auto comment = std::basic_string<TCHAR>( _T("95% ") ) + distance.data();
    data.SetComment(comment.data());
  }
  else {
    data.SetCommentInvalid();
  }
}

void
UpdateInfoBoxTaskAADistanceMin(InfoBoxData &data) noexcept
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
UpdateInfoBoxTaskAASpeed(InfoBoxData &data) noexcept
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets || common_stats.aat_speed_target <= 0) {
    data.SetInvalid();
    return;
  }

  // Set Value and units
  data.SetValueFromTaskSpeed(common_stats.aat_speed_target);
}

void
UpdateInfoBoxTaskAASpeedMax(InfoBoxData &data) noexcept
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets || common_stats.aat_speed_max <= 0) {
    data.SetInvalid();
    return;
  }

  // Set Value and units
  data.SetValueFromTaskSpeed(common_stats.aat_speed_max);
}

void
UpdateInfoBoxTaskAASpeedMin(InfoBoxData &data) noexcept
{
  const auto &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (!task_stats.has_targets ||
      !task_stats.task_valid || common_stats.aat_speed_min <= 0) {
    data.SetInvalid();
    return;
  }

  // Set Value and units
  data.SetValueFromTaskSpeed(common_stats.aat_speed_min);
}

void
UpdateInfoBoxTaskTimeUnderMaxHeight(InfoBoxData &data) noexcept
{
  const auto &calculated = CommonInterface::Calculated();
  const auto &task_stats = calculated.ordered_task_stats;
  const auto &common_stats = calculated.common_stats;
  const double maxheight = backend_components->protected_task_manager->GetOrderedTaskSettings().start_constraints.max_height;

  if (!task_stats.task_valid || maxheight <= 0
      || !backend_components->protected_task_manager
      || !common_stats.TimeUnderStartMaxHeight.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromTimeTwoLines(CommonInterface::Basic().time -
                                common_stats.TimeUnderStartMaxHeight);
  data.SetComment(_("Time Below"));
}

void
UpdateInfoBoxNextETEVMG(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  if (!basic.ground_speed_available || !task_stats.task_valid ||
      !task_stats.current_leg.remaining.IsDefined()) {
    data.SetInvalid();
    return;
  }

  const auto d = task_stats.current_leg.remaining.GetDistance();
  const auto v = basic.ground_speed;

  if (!task_stats.task_valid ||
      v <= 0) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromTimeTwoLines(FloatDuration{d / v});
  data.SetValueColor(5);
}

void
UpdateInfoBoxNextETAVMG(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  if (!basic.ground_speed_available || !task_stats.task_valid ||
      !task_stats.current_leg.remaining.IsDefined()) {
    data.SetInvalid();
    return;
  }

  const auto d = task_stats.current_leg.remaining.GetDistance();
  const auto v = basic.ground_speed;

  if (!task_stats.task_valid ||
      v <= 0) {
    data.SetInvalid();
    return;
  }

  const BrokenTime &now_local = CommonInterface::Calculated().date_time_local;
  if (now_local.IsPlausible()) {
    const std::chrono::seconds dd{long(d/v)};
    const BrokenTime t = now_local + dd;
    data.FmtValue(_T("{:02}:{:02}"), t.hour, t.minute);
    data.FmtComment(_T("{:02}"), t.second);
  }
  
  data.SetValueColor(5);

}

void
UpdateInfoBoxFinalETEVMG(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  if (!basic.ground_speed_available || !task_stats.task_valid ||
      !task_stats.total.remaining.IsDefined()) {
    data.SetInvalid();
    return;
  }

  const auto d = task_stats.total.remaining.GetDistance();
  const auto v = basic.ground_speed;

  if (!task_stats.task_valid ||
      v <= 0) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromTimeTwoLines(FloatDuration{d / v});
}

void
UpdateInfoBoxCruiseEfficiency(InfoBoxData &data) noexcept
{
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  if (!task_stats.task_valid || !task_stats.start.HasStarted()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromPercent(task_stats.cruise_efficiency*100);
  data.SetCommentFromVerticalSpeed(task_stats.effective_mc, false);
}

static constexpr unsigned
SecondsUntil(TimeStamp now, RoughTime until) noexcept
{
  auto d = TimeStamp{until} - now;
  if (d.count() < 0)
    d += std::chrono::hours{24};
  return std::chrono::duration_cast<std::chrono::duration<unsigned>>(d).count();
}

void
UpdateInfoBoxStartOpen(InfoBoxData &data) noexcept
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

  const auto now_s = basic.time;
  const RoughTime now{now_s};

  if (open.HasEnded(now)) {
    data.SetValueInvalid();
    data.SetComment(_("Closed"));
  } else if (open.HasBegun(now)) {
    if (open.GetEnd().IsValid()) {
      unsigned seconds = SecondsUntil(now_s, open.GetEnd());
      data.FmtValue(_T("{:02}:{:02}"), seconds / 60, seconds % 60);
      data.SetValueColor(3);
    } else
      data.SetValueInvalid();

    data.SetComment(_("Open"));
  } else {
    unsigned seconds = SecondsUntil(now_s, open.GetStart());
    data.FmtValue(_T("{:02}:{:02}"), seconds / 60, seconds % 60);
    data.SetValueColor(2);
    data.SetComment(_("Waiting"));
  }
}

void
UpdateInfoBoxStartOpenArrival(InfoBoxData &data) noexcept
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

  const auto arrival_s = basic.time + current_remaining.time_elapsed;
  const RoughTime arrival{arrival_s};

  if (open.HasEnded(arrival)) {
    data.SetValueInvalid();
    data.SetComment(_("Closed"));
  } else if (open.HasBegun(arrival)) {
    if (open.GetEnd().IsValid()) {
      unsigned seconds = SecondsUntil(arrival_s, open.GetEnd());
      data.FmtValue(_T("{:02}:{:02}"), seconds / 60, seconds % 60);
      data.SetValueColor(3);
    } else
      data.SetValueInvalid();

    data.SetComment(_("Open"));
  } else {
    unsigned seconds = SecondsUntil(arrival_s, open.GetStart());
    data.FmtValue(_T("{:02}:{:02}"), seconds / 60, seconds % 60);
    data.SetValueColor(2);
    data.SetComment(_("Waiting"));
  }
}

/*
 * The NextArrow infobox contains an arrow pointing at the next waypoint.
 * This function updates the text fields in the infobox.
 */
void
InfoBoxContentNextArrow::Update(InfoBoxData &data) noexcept
{
  // use proper non-terminal next task stats
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;

  // Check if data is valid
  bool distance_valid = task_stats.task_valid && vector_remaining.IsValid();
  bool angle_valid = distance_valid && basic.track_available;

  // Set title. Use waypoint name if available.
  const auto way_point = backend_components->protected_task_manager
    ? backend_components->protected_task_manager->GetActiveWaypoint()
    : nullptr;
  if (!way_point)
    data.SetTitle(_("Next arrow"));
  else
    data.SetTitle(way_point->name.c_str());

  // Set value
  if (angle_valid)
    // Enables OnCustomPaint
    // TODO: use an appropriate digest
    data.SetCustom(basic.track_available.ToInteger());
  else
    data.SetInvalid();

  // Set comment
  if (distance_valid)
    data.SetCommentFromDistance(vector_remaining.distance);
  else
    data.SetCommentInvalid();
}

/*
 * The NextArrow infobox contains an arrow pointing at the next waypoint.
 * This function renders the arrow.
 */
void
InfoBoxContentNextArrow::OnCustomPaint(Canvas &canvas,
                                       const PixelRect &rc) noexcept
{
  // use proper non-terminal next task stats
  const NMEAInfo &basic = CommonInterface::Basic();
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
  const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;

  // We exit immediately if this function is called when all data isn't
  // available. This can happen e.g. while state is being changed.
  if (!task_stats.task_valid
      || !vector_remaining.IsValid()
      || !basic.track_available)
    return;

  Angle bd = vector_remaining.bearing - basic.track;

  NextArrowRenderer renderer(UIGlobals::GetLook().wind_arrow_info_box);
  renderer.DrawArrow(canvas, rc, bd);
}

/*
 * This infobox shows either AAT dT + ETA, or only ETA depending on task type
 */
void
UpdateInfoTaskETAorAATdT(InfoBoxData& data) noexcept
{
  const auto& calculated = CommonInterface::Calculated();
  const TaskStats& task_stats = calculated.ordered_task_stats;

  // Always call the ETA infobox function. If task is AAT, the value of
  // ETA infobox will be used as the comment of AATdT infobox
  UpdateInfoBoxFinalETA(data);
  if (task_stats.has_targets) { // Is AAT
    // save the HH:MM ETA to use it as a comment of AATdT infobox
    auto eta_text = data.value;
    UpdateInfoBoxTaskAATimeDelta(data);
    data.SetComment(eta_text);

    data.SetTitle(_T("AAT delta time"));
  } else
    data.SetTitle(_T("Task arrival time"));
}
