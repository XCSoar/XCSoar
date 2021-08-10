/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Math/LeastSquares.hpp"
#include "Math/ConvexFilter.hpp"
#include "Math/Histogram.hpp"
#include "thread/Mutex.hxx"
#include "time/FloatDuration.hxx"

class FlightStatistics {
public:
  LeastSquares thermal_average;
  LeastSquares altitude;
  ConvexFilter altitude_base;
  ConvexFilter altitude_ceiling;
  LeastSquares task_speed;
  LeastSquares altitude_terrain;
  Histogram vario_circling_histogram;
  Histogram vario_cruise_histogram;
  mutable Mutex mutex;

  void StartTask();

  double AverageThermalAdjusted(double wthis, const bool circling);
  double GetMinWorkingHeight() const;
  double GetMaxWorkingHeight() const;
  double GetVarioScalePositive() const;
  double GetVarioScaleNegative() const;

  void AddAltitude(FloatDuration tflight,
                   double alt, bool final_glide) noexcept;
  void AddAltitudeTerrain(FloatDuration tflight, double terrainalt) noexcept;
  void AddTaskSpeed(FloatDuration tflight, double val) noexcept;
  void AddClimbBase(FloatDuration tflight, double alt) noexcept;
  void AddClimbCeiling(FloatDuration tflight, double alt) noexcept;
  void AddThermalAverage(FloatDuration tflight_start,
                         FloatDuration tflight_end, double v) noexcept;
  void AddClimbRate(FloatDuration tflight,
                    double vario, bool circling) noexcept;

  void Reset();
};

#endif
