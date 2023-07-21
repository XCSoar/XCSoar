// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightStatisticsRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Look/MapLook.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Engine/Contest/Settings.hpp"
#include "MapSettings.hpp"
#include "Projection/ChartProjection.hpp"
#include "Renderer/TaskRenderer.hpp"
#include "Renderer/TaskPointRenderer.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/MapScaleRenderer.hpp"
#include "Engine/Contest/Solvers/Retrospective.hpp"
#include "Computer/Settings.hpp"

#include <algorithm>

FlightStatisticsRenderer::FlightStatisticsRenderer(const ChartLook &_chart_look,
                                                   const MapLook &_map_look) noexcept
  :chart_look(_chart_look),
   map_look(_map_look),
   airspace_renderer(map_look.airspace),
   trail_renderer(map_look.trail) {}

void
FlightStatisticsRenderer::DrawContestSolution(Canvas &canvas,
                                              const Projection &projection,
                                              const ContestStatistics &statistics,
                                              unsigned i) noexcept
{
  if (!statistics.GetResult(i).IsDefined())
    return;

  canvas.Select(map_look.contest_pens[i]);
  trail_renderer.DrawTraceVector(canvas, projection,
                                 statistics.GetSolution(i));
}

void
FlightStatisticsRenderer::DrawContestTriangle(Canvas &canvas, const Projection &projection,
                                              const ContestStatistics &statistics, unsigned i) noexcept
{
  if (!statistics.GetResult(i).IsDefined() ||
      statistics.GetSolution(i).size() != 5)
    return;

  canvas.Select(map_look.contest_pens[i]);
  canvas.SelectHollowBrush();
  trail_renderer.DrawTriangle(canvas, projection,
                              statistics.GetSolution(i));
}

void
FlightStatisticsRenderer::RenderContest(Canvas &canvas, const PixelRect rc,
                                        const NMEAInfo &nmea_info,
                                        const ComputerSettings &settings_computer,
                                        const MapSettings &settings_map,
                                        const ContestStatistics &contest,
                                        const TraceComputer &trace_computer,
                                        const Retrospective &retrospective) noexcept
{
  ChartRenderer chart(chart_look, canvas, rc);
  chart.Begin();

  if (!trail_renderer.LoadTrace(trace_computer)) {
    chart.DrawNoData();
    chart.Finish();
    return;
  }

  const PixelRect &rc_chart = chart.GetChartRect();
  GeoBounds bounds(nmea_info.location);
  trail_renderer.ScanBounds(bounds);

  /* scan all solutions to make sure they are all visible */
  for (unsigned i = 0; i < 3; ++i) {
    if (contest.GetResult(i).IsDefined()) {
      const ContestTraceVector &solution = contest.GetSolution(i);
      for (auto j = solution.begin(), end = solution.end(); j != end; ++j)
        bounds.Extend(j->location);
    }
  }

  const ChartProjection proj(rc_chart, TaskProjection(bounds));

  background_renderer.Draw(canvas, proj, settings_map.terrain);

  {
#ifndef ENABLE_OPENGL
    BufferCanvas stencil_canvas;
    stencil_canvas.Create(canvas);
#endif

    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           stencil_canvas,
#endif
                           proj, settings_map.airspace);
  }

#ifdef ENABLE_OPENGL
  /* desaturate the map background, to focus on the contest */
  canvas.FadeToWhite(0xc0);
#endif

  {
    // draw place names found in the retrospective task
    //    canvas.Select(*dialog_look.small_font);
    canvas.Select(chart_look.label_font);
    canvas.SetBackgroundTransparent();

    for (const auto &i : retrospective.getNearWaypointList()) {
      auto wp_pos = proj.GeoToScreen(i.waypoint->location);
      canvas.DrawText(wp_pos, i.waypoint->name.c_str());
    }
  }

  auto aircraft_pos = proj.GeoToScreen(nmea_info.location);
  AircraftRenderer::Draw(canvas, settings_map, map_look.aircraft,
                         nmea_info.attitude.heading, aircraft_pos);

  trail_renderer.Draw(canvas, proj);

  switch (settings_computer.contest.contest) {
  case Contest::NONE:
    break;

  case Contest::OLC_SPRINT:
  case Contest::OLC_CLASSIC:
  case Contest::SIS_AT:
  case Contest::NET_COUPE:
  case Contest::DMST:
    DrawContestSolution(canvas, proj, contest, 0);
    break;

  case Contest::OLC_FAI:
    DrawContestTriangle(canvas, proj, contest, 0);
    break;

  case Contest::OLC_LEAGUE:
    /* draw classic first, and triangle on top of it */
    DrawContestSolution(canvas, proj, contest, 1);
    DrawContestSolution(canvas, proj, contest, 0);
    break;

  case Contest::OLC_PLUS:
  case Contest::XCONTEST:
  case Contest::DHV_XC:
    DrawContestSolution(canvas, proj, contest, 0);
    DrawContestTriangle(canvas, proj, contest, 1);
    break;

  case Contest::WEGLIDE_FREE:
    DrawContestSolution(canvas, proj, contest, 0);
    break;

  case Contest::WEGLIDE_DISTANCE:
  case Contest::WEGLIDE_FAI:

  case Contest::WEGLIDE_OR:
    DrawContestSolution(canvas, proj, contest, 0);
    break;

  case Contest::CHARRON:
    DrawContestSolution(canvas, proj, contest, 0);
    break;

  }

  RenderMapScale(canvas, proj, rc_chart, map_look.overlay);

  chart.Finish();
}

void
FlightStatisticsRenderer::CaptionContest(TCHAR *sTmp,
                                         const ContestSettings &settings,
                                         const DerivedInfo &derived) noexcept
{
  if (settings.contest == Contest::OLC_PLUS) {
    const ContestResult& result =
        derived.contest_stats.GetResult(2);

    const ContestResult& result_classic =
        derived.contest_stats.GetResult(0);

    const ContestResult& result_fai =
        derived.contest_stats.GetResult(1);

    StringFormatUnsafe(sTmp,
                       (Layout::landscape
                        ? _T("%s:\r\n%s\r\n%s (FAI)\r\n%s:\r\n%.1f %s\r\n%s: %s\r\n%s: %s\r\n")
                        : _T("%s: %s\r\n%s (FAI)\r\n%s: %.1f %s\r\n%s: %s\r\n%s: %s\r\n")),
                       _("Distance"),
                       FormatUserDistanceSmart(result_classic.distance).c_str(),
                       FormatUserDistanceSmart(result_fai.distance).c_str(),
                       _("Score"), (double)result.score, _("pts"),
                       _("Time"),
                       FormatSignedTimeHHMM(result.time).c_str(),
                       _("Speed"), FormatUserTaskSpeed(result.GetSpeed()).c_str());
  } else if (settings.contest == Contest::DHV_XC ||
             settings.contest == Contest::XCONTEST) {
    const ContestResult& result_free =
      derived.contest_stats.GetResult(0);

    const ContestResult& result_triangle =
      derived.contest_stats.GetResult(1);

    StringFormatUnsafe(sTmp,
                       (Layout::landscape
                        ? _T("%s:\r\n%s (Free)\r\n%s (Triangle)\r\n%s:\r\n%.1f %s\r\n%s: %s\r\n%s: %s\r\n")
                        : _T("%s: %s (Free)\r\n%s (Triangle)\r\n%s: %.1f %s\r\n%s: %s\r\n%s: %s\r\n")),
                       _("Distance"),
                       FormatUserDistanceSmart(result_free.distance).c_str(),
                       FormatUserDistanceSmart(result_triangle.distance).c_str(),
                       _("Score"), (double)result_free.score, _("pts"),
                       _("Time"),
                       FormatSignedTimeHHMM(result_free.time).c_str(),
                       _("Speed"),
                       FormatUserTaskSpeed(result_free.GetSpeed()).c_str());
  } else {
    unsigned result_index;
    switch (settings.contest) {
    case Contest::OLC_LEAGUE:
      result_index = 0;
      break;

    default:
      result_index = -1;
      break;
    }

    const ContestResult& result_olc =
        derived.contest_stats.GetResult(result_index);

    StringFormatUnsafe(sTmp,
                       (Layout::landscape
                        ? _T("%s:\r\n%s\r\n%s:\r\n%.1f %s\r\n%s: %s\r\n%s: %s\r\n")
                        : _T("%s: %s\r\n%s: %.1f %s\r\n%s: %s\r\n%s: %s\r\n")),
                       _("Distance"),
                       FormatUserDistanceSmart(result_olc.distance).c_str(),
                       _("Score"), (double)result_olc.score, _("pts"),
                       _("Time"),
                       FormatSignedTimeHHMM(result_olc.time).c_str(),
                       _("Speed"),
                       FormatUserTaskSpeed(result_olc.GetSpeed()).c_str());
  }
}

void
FlightStatisticsRenderer::RenderTask(Canvas &canvas, const PixelRect rc,
                                     const NMEAInfo &nmea_info,
                                     [[maybe_unused]] const ComputerSettings &settings_computer,
                                     const MapSettings &settings_map,
                                     const TaskStats &task_stats,
                                     const ProtectedTaskManager &_task_manager,
                                     const TraceComputer *trace_computer) noexcept
{
  ChartRenderer chart(chart_look, canvas, rc);
  chart.Begin();

  if (!task_stats.task_valid || !task_stats.bounds.IsValid()) {
    chart.DrawNoData();
    chart.Finish();
    return;
  }

  const PixelRect &rc_chart = chart.GetChartRect();
  const ChartProjection proj{rc_chart, TaskProjection{task_stats.bounds}, 1};

  background_renderer.Draw(canvas, proj, settings_map.terrain);

  {
#ifndef ENABLE_OPENGL
    BufferCanvas stencil_canvas;
    stencil_canvas.Create(canvas);
#endif

    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           stencil_canvas,
#endif
                           proj, settings_map.airspace);
  }

#ifdef ENABLE_OPENGL
  /* desaturate the map background, to focus on the task */
  canvas.FadeToWhite(0xc0);
#endif

  {
    ProtectedTaskManager::Lease task_manager(_task_manager);
    const OrderedTask &task = task_manager->GetOrderedTask();

    if (IsError(task.CheckTask())) {
      chart.DrawNoData();
      chart.Finish();
      return;
    }

    OZRenderer ozv(map_look.task, map_look.airspace, settings_map.airspace);
    TaskPointRenderer tpv(canvas, proj, map_look.task,
                          task.GetTaskProjection(),
                          ozv, false, TaskPointRenderer::TargetVisibility::ALL,
                          nmea_info.GetLocationOrInvalid());
    ::TaskRenderer dv(tpv, proj.GetScreenBounds());
    dv.Draw(task);
  }

  if (trace_computer != nullptr)
    trail_renderer.Draw(canvas, *trace_computer, proj);

  if (nmea_info.location_available) {
    auto aircraft_pos = proj.GeoToScreen(nmea_info.location);
    AircraftRenderer::Draw(canvas, settings_map, map_look.aircraft,
                           nmea_info.attitude.heading, aircraft_pos);
  }

  RenderMapScale(canvas, proj, rc_chart, map_look.overlay);

  chart.Finish();
}

void
FlightStatisticsRenderer::CaptionTask(TCHAR *sTmp, const DerivedInfo &derived) noexcept
{
  const TaskStats &task_stats = derived.ordered_task_stats;
  const CommonStats &common = derived.common_stats;

  if (!task_stats.task_valid ||
      !derived.task_stats.total.remaining.IsDefined()) {
    _tcscpy(sTmp, _("No task"));
  } else {
    const auto d_remaining = derived.task_stats.total.remaining.GetDistance();
    if (task_stats.has_targets) {
      const auto timetext1 = FormatSignedTimeHHMM(task_stats.total.time_remaining_start);
      const auto timetext2 = FormatSignedTimeHHMM(common.aat_time_remaining);

      if (Layout::landscape) {
        StringFormatUnsafe(sTmp,
                           _T("%s:\r\n  %s\r\n%s:\r\n  %s\r\n%s:\r\n  %5.0f %s\r\n%s:\r\n  %5.0f %s\r\n"),
                           _("Task to go"), timetext1.c_str(),
                           _("AAT to go"), timetext2.c_str(),
                           _("Distance to go"),
                           (double)Units::ToUserDistance(d_remaining),
                           Units::GetDistanceName(), _("Target speed"),
                           (double)Units::ToUserTaskSpeed(common.aat_speed_target),
                           Units::GetTaskSpeedName());
      } else {
        StringFormatUnsafe(sTmp,
                           _T("%s: %s\r\n%s: %s\r\n%s: %5.0f %s\r\n%s: %5.0f %s\r\n"),
                           _("Task to go"), timetext1.c_str(),
                           _("AAT to go"), timetext2.c_str(),
                           _("Distance to go"),
                           (double)Units::ToUserDistance(d_remaining),
                           Units::GetDistanceName(),
                           _("Target speed"),
                           (double)Units::ToUserTaskSpeed(common.aat_speed_target),
                           Units::GetTaskSpeedName());
      }
    } else {
      StringFormatUnsafe(sTmp, _T("%s: %s\r\n%s: %5.0f %s\r\n"),
                         _("Task to go"),
                         FormatSignedTimeHHMM(task_stats.total.time_remaining_now).c_str(),
                         _("Distance to go"),
                         (double)Units::ToUserDistance(d_remaining),
                         Units::GetDistanceName());
    }
  }
}
