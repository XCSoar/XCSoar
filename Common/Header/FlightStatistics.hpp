/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "Task.h"
#include "Screen/Canvas.hpp"
#include "Thread/Mutex.hpp"

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
  void RenderAirspace(Canvas &canvas, const RECT rc);
  void RenderBarograph(Canvas &canvas, const RECT rc);
  void RenderClimb(Canvas &canvas, const RECT rc);
  void RenderGlidePolar(Canvas &canvas, const RECT rc);
  void RenderWind(Canvas &canvas, const RECT rc);
  void RenderTemperature(Canvas &canvas, const RECT rc);
  void RenderTask(Canvas &canvas, const RECT rc, const bool olcmode);
  void RenderSpeed(Canvas &canvas, const RECT rc);
  void CaptionBarograph( TCHAR *sTmp);
  void CaptionClimb( TCHAR* sTmp);
  void CaptionPolar( TCHAR *sTmp);
  void CaptionTempTrace( TCHAR *sTmp);
  void CaptionTask( TCHAR *sTmp);
private:
  LeastSquares ThermalAverage;
  LeastSquares Wind_x;
  LeastSquares Wind_y;
  LeastSquares Altitude;
  LeastSquares Altitude_Base;
  LeastSquares Altitude_Ceiling;
  LeastSquares Task_Speed;
  double       LegStartTime[MAXTASKPOINTS];
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
