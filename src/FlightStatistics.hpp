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

#ifndef FLIGHT_STATISTICS_HPP
#define FLIGHT_STATISTICS_HPP

#include "Math/leastsqs.h"
#include "Thread/Mutex.hpp"

class FlightStatistics {
public:
  LeastSquares ThermalAverage;
  LeastSquares Altitude;
  LeastSquares Altitude_Base;
  LeastSquares Altitude_Ceiling;
  LeastSquares Task_Speed;
  LeastSquares Altitude_Terrain;
  mutable Mutex mutexStats;

  void StartTask();

  fixed AverageThermalAdjusted(const fixed wthis, const bool circling);

  void AddAltitude(const fixed tflight, const fixed alt);
  void AddAltitudeTerrain(const fixed tflight, const fixed terrainalt);
  void AddTaskSpeed(const fixed tflight, const fixed val);
  void AddClimbBase(const fixed tflight, const fixed alt);
  void AddClimbCeiling(const fixed tflight, const fixed alt);
  void AddThermalAverage(const fixed v);

  void Reset();
};

#endif
