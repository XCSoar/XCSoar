// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  void StartTask() noexcept;

  [[gnu::pure]]
  double AverageThermalAdjusted(double wthis, bool circling) noexcept;

  [[gnu::pure]]
  double GetMinWorkingHeight() const noexcept;

  [[gnu::pure]]
  double GetMaxWorkingHeight() const noexcept;

  [[gnu::pure]]
  double GetVarioScalePositive() const noexcept;

  [[gnu::pure]]
  double GetVarioScaleNegative() const noexcept;

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

  void Reset() noexcept;
};
