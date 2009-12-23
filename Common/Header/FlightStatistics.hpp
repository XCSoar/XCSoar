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

#ifndef FLIGHT_STATISTICS_HPP
#define FLIGHT_STATISTICS_HPP

#include "Math/leastsqs.h"
#include "Thread/Mutex.hpp"
#include "Sizes.h"

#include <windef.h>

struct NMEA_INFO;
struct DERIVED_INFO;
struct SETTINGS_COMPUTER;
struct SETTINGS_MAP;
class Canvas;
class OLCOptimizer;
class WindStore;
class Airspaces;
class RasterTerrain;
class GlidePolar;

class FlightStatistics {
public:
  void StartTask(double starttime);
  double AverageThermalAdjusted(const double wthis, const bool circling);
  void AddAltitude(const double tflight,
		   const double alt);
  void AddAltitudeTerrain(const double tflight,
			  const double terrainalt);
  void SaveTaskSpeed(const double val);
  void SetLegStart(const int activewaypoint,
		   const double time);
  void AddClimbBase(const double tflight,
		    const double alt);
  void AddClimbCeiling(const double tflight,
		       const double alt);
  void AddThermalAverage(const double v);
public:
  void Reset();
  void RenderAirspace(Canvas &canvas, const RECT rc,
                      const NMEA_INFO &nmea_info, const DERIVED_INFO &derived,
                      const SETTINGS_MAP &settings_map,
                      const Airspaces &airspace_database,
                      RasterTerrain &terrain) const;
  void RenderBarograph(Canvas &canvas, const RECT rc,
                       const DERIVED_INFO &derived) const;
  void RenderClimb(Canvas &canvas, const RECT rc,
                   const GlidePolar& glide_polar) const;
  void RenderGlidePolar(Canvas &canvas, const RECT rc,
                        const DERIVED_INFO &derived,
                        const SETTINGS_COMPUTER &settings_computer,
                        const GlidePolar& glide_polar) const;
  void RenderWind(Canvas &canvas, const RECT rc,
                  const NMEA_INFO &nmea_info,
                  const WindStore &wind_store) const;
  void RenderTemperature(Canvas &canvas, const RECT rc) const;
  void RenderTask(Canvas &canvas, const RECT rc,
                  const NMEA_INFO &nmea_info,
                  const SETTINGS_COMPUTER &settings_computer,
                  const SETTINGS_MAP &settings_map,
                  const OLCOptimizer &olc, bool olcmode) const;
  void RenderSpeed(Canvas &canvas, const RECT rc,
                   const DERIVED_INFO &derived) const;
  void CaptionBarograph( TCHAR *sTmp);
  void CaptionClimb( TCHAR* sTmp);
  void CaptionPolar(TCHAR * sTmp) const;
  void CaptionTempTrace(TCHAR *sTmp) const;
  void CaptionTask(TCHAR *sTmp,
                   const DERIVED_INFO &derived) const;
private:
  LeastSquares ThermalAverage;
  LeastSquares Wind_x;
  LeastSquares Wind_y;
  LeastSquares Altitude;
  LeastSquares Altitude_Base;
  LeastSquares Altitude_Ceiling;
  LeastSquares Task_Speed;
#ifdef OLD_TASK
  double       LegStartTime[MAXTASKPOINTS];
#endif
  LeastSquares Altitude_Terrain;
  Mutex mutexStats;
  void Lock() {
    mutexStats.Lock();
  }
  void Unlock() {
    mutexStats.Unlock();
  }
};

#endif
