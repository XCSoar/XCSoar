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

#include "FlightStatisticsRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Util/Macros.hpp"
#include "Look/MapLook.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Screen/Canvas.hpp"
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
#include "Engine/Contest/Solvers/Retrospective.hpp"
#include "Computer/Settings.hpp"

#include <algorithm>

#include <stdio.h>

using std::max;

FlightStatisticsRenderer::FlightStatisticsRenderer(const ChartLook &_chart_look,
                                                   const MapLook &_map_look)
  :chart_look(_chart_look),
   map_look(_map_look),
   trail_renderer(map_look.trail) {}

void
FlightStatisticsRenderer::DrawContestSolution(Canvas &canvas,
                                              const Projection &projection,
                                              const ContestStatistics &statistics,
                                              unsigned i) const
{
  if (!statistics.GetResult(i).IsDefined())
    return;

  canvas.Select(map_look.contest_pens[i]);
  trail_renderer.DrawTraceVector(canvas, projection,
                                 statistics.GetSolution(i));
}

void
FlightStatisticsRenderer::DrawContestTriangle(Canvas &canvas, const Projection &projection,
                                              const ContestStatistics &statistics, unsigned i) const
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
FlightStatisticsRenderer::RenderOLC(Canvas &canvas, const PixelRect rc,
                                    const NMEAInfo &nmea_info,
                                    const ComputerSettings &settings_computer,
                                    const MapSettings &settings_map,
                                    const ContestStatistics &contest,
                                    const TraceComputer &trace_computer,
                                    const Retrospective &retrospective) const
{
  if (!trail_renderer.LoadTrace(trace_computer)) {
    ChartRenderer chart(chart_look, canvas, rc);
    chart.DrawNoData();
    return;
  }

  TaskProjection task_projection(trail_renderer.GetBounds(nmea_info.location));

  /* scan all solutions to make sure they are all visible */
  for (unsigned i = 0; i < 3; ++i) {
    if (contest.GetResult(i).IsDefined()) {
      const ContestTraceVector &solution = contest.GetSolution(i);
      for (auto j = solution.begin(), end = solution.end(); j != end; ++j)
        task_projection.Scan(j->location);
    }
  }


  const ChartProjection proj(rc, task_projection);

  {
    // draw place names found in the retrospective task
    //    canvas.Select(*dialog_look.small_font);
    canvas.Select(chart_look.label_font);
    canvas.SetBackgroundTransparent();

    for (const auto &i : retrospective.getNearWaypointList()) {
      RasterPoint wp_pos = proj.GeoToScreen(i.waypoint.location);
      canvas.DrawText(wp_pos.x,
                      wp_pos.y,
                      i.waypoint.name.c_str());
    }
  }

  RasterPoint aircraft_pos = proj.GeoToScreen(nmea_info.location);
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
  }
}

void
FlightStatisticsRenderer::CaptionOLC(TCHAR *sTmp,
                                     const ContestSettings &settings,
                                     const DerivedInfo &derived)
{
  if (settings.contest == Contest::OLC_PLUS) {
    const ContestResult& result =
        derived.contest_stats.GetResult(2);

    const ContestResult& result_classic =
        derived.contest_stats.GetResult(0);

    const ContestResult& result_fai =
        derived.contest_stats.GetResult(1);

    TCHAR timetext1[100];
    FormatSignedTimeHHMM(timetext1, (int)result.time);
    TCHAR distance_classic[100];
    FormatUserDistanceSmart(result_classic.distance, distance_classic, 100);
    TCHAR distance_fai[100];
    FormatUserDistanceSmart(result_fai.distance, distance_fai, 100);
    TCHAR speed[100];
    FormatUserTaskSpeed(result.GetSpeed(), speed, ARRAY_SIZE(speed));
    _stprintf(sTmp,
              (Layout::landscape
               ? _T("%s:\r\n%s\r\n%s (FAI)\r\n%s:\r\n%.1f %s\r\n%s: %s\r\n%s: %s\r\n")
               : _T("%s: %s\r\n%s (FAI)\r\n%s: %.1f %s\r\n%s: %s\r\n%s: %s\r\n")),
              _("Distance"), distance_classic, distance_fai,
              _("Score"), (double)result.score, _("pts"),
              _("Time"), timetext1,
              _("Speed"), speed);
  } else if (settings.contest == Contest::DHV_XC ||
             settings.contest == Contest::XCONTEST) {
    const ContestResult& result_free =
        derived.contest_stats.GetResult(0);

    const ContestResult& result_triangle =
        derived.contest_stats.GetResult(1);

    TCHAR timetext1[100];
    FormatSignedTimeHHMM(timetext1, (int)result_free.time);
    TCHAR distance[100];
    FormatUserDistanceSmart(result_free.distance, distance, 100);
    TCHAR distance_fai[100];
    FormatUserDistanceSmart(result_triangle.distance, distance_fai, 100);
    TCHAR speed[100];
    FormatUserTaskSpeed(result_free.GetSpeed(), speed, ARRAY_SIZE(speed));
    _stprintf(sTmp,
              (Layout::landscape
               ? _T("%s:\r\n%s (Free)\r\n%s (Triangle)\r\n%s:\r\n%.1f %s\r\n%s: %s\r\n%s: %s\r\n")
               : _T("%s: %s (Free)\r\n%s (Triangle)\r\n%s: %.1f %s\r\n%s: %s\r\n%s: %s\r\n")),
              _("Distance"), distance, distance_fai,
              _("Score"), (double)result_free.score, _("pts"),
              _("Time"), timetext1,
              _("Speed"), speed);
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

    TCHAR timetext1[100];
    FormatSignedTimeHHMM(timetext1, (int)result_olc.time);
    TCHAR distance[100];
    FormatUserDistanceSmart(result_olc.distance, distance, 100);
    TCHAR speed[100];
    FormatUserTaskSpeed(result_olc.GetSpeed(), speed, ARRAY_SIZE(speed));
    _stprintf(sTmp,
              (Layout::landscape
               ? _T("%s:\r\n%s\r\n%s:\r\n%.1f %s\r\n%s: %s\r\n%s: %s\r\n")
               : _T("%s: %s\r\n%s: %.1f %s\r\n%s: %s\r\n%s: %s\r\n")),
              _("Distance"), distance,
              _("Score"), (double)result_olc.score, _("pts"),
              _("Time"), timetext1,
              _("Speed"), speed);
  }
}

void
FlightStatisticsRenderer::RenderTask(Canvas &canvas, const PixelRect rc,
                                     const NMEAInfo &nmea_info,
                                     const ComputerSettings &settings_computer,
                                     const MapSettings &settings_map,
                                     const ProtectedTaskManager &_task_manager,
                                     const TraceComputer *trace_computer) const
{
  ChartRenderer chart(chart_look, canvas, rc);

  ChartProjection proj;

  {
    ProtectedTaskManager::Lease task_manager(_task_manager);
    const OrderedTask &task = task_manager->GetOrderedTask();

    if (!task.CheckTask()) {
      chart.DrawNoData();
      return;
    }

    proj.Set(rc, task, nmea_info.location);

    OZRenderer ozv(map_look.task, map_look.airspace, settings_map.airspace);
    TaskPointRenderer tpv(canvas, proj, map_look.task,
                          task.GetTaskProjection(),
                          ozv, false, TaskPointRenderer::ALL,
                          nmea_info.location_available
                          ? nmea_info.location : GeoPoint::Invalid());
    ::TaskRenderer dv(tpv, proj.GetScreenBounds());
    dv.Draw(task);
  }

  if (trace_computer != NULL)
    trail_renderer.Draw(canvas, *trace_computer, proj, 0);

  if (nmea_info.location_available) {
    RasterPoint aircraft_pos = proj.GeoToScreen(nmea_info.location);
    AircraftRenderer::Draw(canvas, settings_map, map_look.aircraft,
                           nmea_info.attitude.heading, aircraft_pos);
  }
}

void
FlightStatisticsRenderer::CaptionTask(TCHAR *sTmp, const DerivedInfo &derived)
{
  const TaskStats &task_stats = derived.ordered_task_stats;
  const CommonStats &common = derived.common_stats;

  if (!task_stats.task_valid ||
      !derived.task_stats.total.remaining.IsDefined()) {
    _tcscpy(sTmp, _("No task"));
  } else {
    const fixed d_remaining = derived.task_stats.total.remaining.GetDistance();
    TCHAR timetext1[100];
    TCHAR timetext2[100];
    if (task_stats.has_targets) {
      FormatSignedTimeHHMM(timetext1, (int)task_stats.total.time_remaining_start);
      FormatSignedTimeHHMM(timetext2, (int)common.aat_time_remaining);

      if (Layout::landscape) {
        _stprintf(sTmp,
            _T("%s:\r\n  %s\r\n%s:\r\n  %s\r\n%s:\r\n  %5.0f %s\r\n%s:\r\n  %5.0f %s\r\n"),
            _("Task to go"), timetext1, _("AAT to go"), timetext2,
            _("Distance to go"),
            (double)Units::ToUserDistance(d_remaining),
            Units::GetDistanceName(), _("Target speed"),
            (double)Units::ToUserTaskSpeed(common.aat_speed_remaining),
            Units::GetTaskSpeedName());
      } else {
        _stprintf(sTmp,
            _T("%s: %s\r\n%s: %s\r\n%s: %5.0f %s\r\n%s: %5.0f %s\r\n"),
            _("Task to go"), timetext1, _("AAT to go"), timetext2,
            _("Distance to go"),
            (double)Units::ToUserDistance(d_remaining),
            Units::GetDistanceName(),
            _("Target speed"),
            (double)Units::ToUserTaskSpeed(common.aat_speed_remaining),
            Units::GetTaskSpeedName());
      }
    } else {
      FormatSignedTimeHHMM(timetext1, (int)task_stats.total.time_remaining_now);
      _stprintf(sTmp, _T("%s: %s\r\n%s: %5.0f %s\r\n"),
                _("Task to go"), timetext1, _("Distance to go"),
                (double)Units::ToUserDistance(d_remaining),
                Units::GetDistanceName());
    }
  }
}
