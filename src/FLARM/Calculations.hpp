// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Id.hpp"
#include "Computer/ClimbAverageCalculator.hpp"

#include <map>

class TimeStamp;
struct TrafficList;

class FlarmCalculations
{
private:
  typedef std::map<FlarmId, ClimbAverageCalculator> AverageCalculatorMap;
  AverageCalculatorMap averageCalculatorMap;

public:
  using AverageResult = ClimbAverageResult;

  /** Sampling rules shared by the climb average and thermal geometry. */
  static constexpr FloatDuration AVERAGE_TIME{30};
  static constexpr ClimbSamplePolicy SAMPLE_POLICY{
    FloatDuration{0.25},
    FloatDuration{5},
  };

  /**
   * Calculates the 30-second average and exposes the actual sample span.
   */
  [[nodiscard]]
  AverageResult Average30sWithSpan(FlarmId flarmId, TimeStamp curTime,
                                   double curAltitude);

  double Average30s(FlarmId flarmId, TimeStamp curTime,
                    double curAltitude) noexcept;

  void Reset(FlarmId flarmId) noexcept;
  void ResetMissing(const TrafficList &traffic) noexcept;
  void Clear() noexcept;
  void CleanUp(TimeStamp now) noexcept;
};
