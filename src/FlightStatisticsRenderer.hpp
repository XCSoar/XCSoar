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

#ifndef FLIGHT_STATISTICS_RENDERER_HPP
#define FLIGHT_STATISTICS_RENDERER_HPP

#include "Math/leastsqs.h"
#include "Navigation/TracePoint.hpp"
#include "FlightStatistics.hpp"
#include "Screen/Point.hpp"

#include <tchar.h>

struct NMEA_INFO;
struct DERIVED_INFO;
struct SETTINGS_COMPUTER;
struct SETTINGS_MAP;
class Canvas;
class WindStore;
class Airspaces;
class RasterTerrain;
class GlidePolar;
class Chart;
class TaskManager;
class ProtectedTaskManager;
struct ContestStatistics;

class FlightStatisticsRenderer {
  FlightStatistics &fs;

public:
  FlightStatisticsRenderer(FlightStatistics &_flight_statistics)
    :fs(_flight_statistics) {}

public:
  void RenderBarograph(Canvas &canvas, const PixelRect rc,
                       const NMEA_INFO &nmea_info,
                       const DERIVED_INFO &derived_info,
                       const ProtectedTaskManager *task) const;

  void RenderBarographSpark(Canvas &canvas, const PixelRect rc,
                            const NMEA_INFO &nmea_info,
                            const DERIVED_INFO &derived_info,
                            const ProtectedTaskManager *task) const;

  void RenderClimb(Canvas &canvas, const PixelRect rc,
                   const GlidePolar& glide_polar) const;

  void RenderGlidePolar(Canvas &canvas, const PixelRect rc,
                        const DERIVED_INFO &derived,
                        const SETTINGS_COMPUTER &settings_computer,
                        const GlidePolar& glide_polar) const;

  void RenderWind(Canvas &canvas, const PixelRect rc,
                  const NMEA_INFO &nmea_info,
                  const WindStore &wind_store) const;

  void RenderTemperature(Canvas &canvas, const PixelRect rc) const;


  void RenderTrace(Canvas &canvas, const PixelRect rc,
                   const NMEA_INFO &nmea_info,
                   const SETTINGS_COMPUTER &settings_computer,
                   const SETTINGS_MAP &settings_map,
                   const TracePointVector& trace) const;

  void RenderOLC(Canvas &canvas, const PixelRect rc,
                 const NMEA_INFO &nmea_info,
                 const DERIVED_INFO &calculated,
                 const SETTINGS_COMPUTER &settings_computer,
                 const ContestStatistics &contest,
                 const TracePointVector& trace) const;

  void RenderTask(Canvas &canvas, const PixelRect rc,
                  const NMEA_INFO &nmea_info,
                  const DERIVED_INFO &derived_info,
                  const SETTINGS_COMPUTER &settings_computer,
                  const SETTINGS_MAP &settings_map,
                  const TaskManager &task) const;

  void RenderSpeed(Canvas &canvas, const PixelRect rc,
                   const NMEA_INFO &nmea_info,
                   const DERIVED_INFO &derived_info,
                   const TaskManager &task) const;

  void CaptionBarograph(TCHAR *sTmp);
  void CaptionClimb(TCHAR* sTmp);
  void CaptionPolar(TCHAR * sTmp, const GlidePolar& glide_polar) const;

  void CaptionTempTrace(TCHAR *sTmp) const;
  void CaptionTask(TCHAR *sTmp, const DERIVED_INFO &derived) const;
  void CaptionOLC(TCHAR *sTmp, const SETTINGS_COMPUTER &settings_computer,
                  const DERIVED_INFO &derived) const;
};

#endif
