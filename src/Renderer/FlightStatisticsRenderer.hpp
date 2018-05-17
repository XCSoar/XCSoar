/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef FLIGHT_STATISTICS_RENDERER_HPP
#define FLIGHT_STATISTICS_RENDERER_HPP

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
  void RenderOLC(Canvas &canvas, const PixelRect rc,
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
  static void CaptionOLC(TCHAR *sTmp, const ContestSettings &settings,
                         const DerivedInfo &derived);

private:
  void DrawContestSolution(Canvas &canvas, const Projection &projection,
                           const ContestStatistics &statistics, unsigned i) const;
  void DrawContestTriangle(Canvas &canvas, const Projection &projection,
                           const ContestStatistics &statistics, unsigned i) const;
};

#endif
