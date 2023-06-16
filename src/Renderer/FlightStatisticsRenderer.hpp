// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BackgroundRenderer.hpp"
#include "AirspaceRenderer.hpp"
#include "TrailRenderer.hpp"

#include <tchar.h>

struct PixelRect;
struct NMEAInfo;
struct DerivedInfo;
struct ContestSettings;
struct ComputerSettings;
struct MapSettings;
class TaskStats;
class Canvas;
class ProtectedTaskManager;
class TraceComputer;
class Retrospective;
struct ContestStatistics;
struct ChartLook;
struct MapLook;

class FlightStatisticsRenderer {
  const ChartLook &chart_look;
  const MapLook &map_look;

  BackgroundRenderer background_renderer;

  AirspaceRenderer airspace_renderer;

  TrailRenderer trail_renderer;

public:
  FlightStatisticsRenderer(const ChartLook &_chart_look,
                           const MapLook &_map_look) noexcept;

  void SetTerrain(const RasterTerrain *terrain) noexcept {
    background_renderer.SetTerrain(terrain);
  }

  void SetAirspaces(const Airspaces *_airspaces) noexcept {
    airspace_renderer.SetAirspaces(_airspaces);
  }

  void RenderContest(Canvas &canvas, const PixelRect rc,
                     const NMEAInfo &nmea_info,
                     const ComputerSettings &settings_computer,
                     const MapSettings &settings_map,
                     const ContestStatistics &contest,
                     const TraceComputer &trace_computer,
                     const Retrospective &retrospective) noexcept;

  void RenderTask(Canvas &canvas, const PixelRect rc,
                  const NMEAInfo &nmea_info,
                  const ComputerSettings &settings_computer,
                  const MapSettings &settings_map,
                  const TaskStats &task_stats,
                  const ProtectedTaskManager &task,
                  const TraceComputer *trace_computer) noexcept;

  static void CaptionTask(TCHAR *sTmp, const DerivedInfo &derived) noexcept;
  static void CaptionContest(TCHAR *sTmp, const ContestSettings &settings,
                             const DerivedInfo &derived) noexcept;

private:
  void DrawContestSolution(Canvas &canvas, const Projection &projection,
                           const ContestStatistics &statistics, unsigned i) noexcept;
  void DrawContestTriangle(Canvas &canvas, const Projection &projection,
                           const ContestStatistics &statistics, unsigned i) noexcept;
};
