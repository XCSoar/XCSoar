// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Id.hpp"
#include "Computer/ClimbAverageCalculator.hpp"

#include <map>

class TimeStamp;

class FlarmCalculations
{
private:
  typedef std::map<FlarmId, ClimbAverageCalculator> AverageCalculatorMap;
  AverageCalculatorMap averageCalculatorMap;

public:
  double Average30s(FlarmId flarmId, TimeStamp curTime,
                    double curAltitude) noexcept;

  void CleanUp(TimeStamp now) noexcept;
};
