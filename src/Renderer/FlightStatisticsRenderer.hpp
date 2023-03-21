// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/TrailRenderer.hpp"

#include <tchar.h>

struct PixelRect;
struct NMEAInfo;
struct DerivedInfo;
struct ContestSettings;
struct ComputerSettings;
struct MapSettings;
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

  mutable TrailRenderer trail_renderer;

public:
  FlightStatisticsRenderer(const ChartLook &_chart_look,
                           const MapLook &_map_look);

public:
  void RenderContest(Canvas &canvas, const PixelRect rc,
                     const NMEAInfo &nmea_info,
                     const ComputerSettings &settings_computer,
                     const MapSettings &settings_map,
                     const ContestStatistics &contest,
                     const TraceComputer &trace_computer,
                     const Retrospective &retrospective) const;

  void RenderTask(Canvas &canvas, const PixelRect rc,
                  const NMEAInfo &nmea_info,
                  const ComputerSettings &settings_computer,
                  const MapSettings &settings_map,
                  const ProtectedTaskManager &task,
                  const TraceComputer *trace_computer) const;

  static void CaptionTask(TCHAR *sTmp, const DerivedInfo &derived);
  static void CaptionContest(TCHAR *sTmp, const ContestSettings &settings,
                             const DerivedInfo &derived);

private:
  void DrawContestSolution(Canvas &canvas, const Projection &projection,
                           const ContestStatistics &statistics, unsigned i) const;
  void DrawContestTriangle(Canvas &canvas, const Projection &projection,
                           const ContestStatistics &statistics, unsigned i) const;
};
