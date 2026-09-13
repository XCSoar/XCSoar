// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Calculations.hpp"
#include "List.hpp"

FlarmCalculations::AverageResult
FlarmCalculations::Average30sWithSpan(FlarmId id, TimeStamp time,
                                      double altitude)
{
  auto [it, inserted] = averageCalculatorMap.try_emplace(id);
  if (inserted)
    it->second.Reset();

  return it->second.GetAverageWithSpan(time, altitude, AVERAGE_TIME,
                                       SAMPLE_POLICY);
}

double
FlarmCalculations::Average30s(FlarmId id, TimeStamp time,
                              double altitude) noexcept
{
  return Average30sWithSpan(id, time, altitude).average;
}

void
FlarmCalculations::Reset(FlarmId id) noexcept
{
  averageCalculatorMap.erase(id);
}

void
FlarmCalculations::ResetMissing(const TrafficList &traffic) noexcept
{
  for (auto it = averageCalculatorMap.begin();
       it != averageCalculatorMap.end();) {
    if (traffic.FindTraffic(it->first) == nullptr)
      it = averageCalculatorMap.erase(it);
    else
      ++it;
  }
}

void
FlarmCalculations::Clear() noexcept
{
  averageCalculatorMap.clear();
}

void
FlarmCalculations::CleanUp(TimeStamp now) noexcept
{
  constexpr FloatDuration MAX_AGE = std::chrono::minutes{1};

  // Iterate through ClimbAverageCalculators and remove expired ones
  for (auto it = averageCalculatorMap.begin(),
       it_end = averageCalculatorMap.end(); it != it_end;)
    if (it->second.Expired(now, MAX_AGE))
      it = averageCalculatorMap.erase(it);
    else
      ++it;
}
