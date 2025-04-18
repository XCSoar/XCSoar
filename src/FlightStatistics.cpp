// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightStatistics.hpp"

using namespace std::chrono;

static constexpr double
ToHours(FloatDuration t) noexcept
{
  return duration_cast<duration<double, hours::period>>(t).count();
}

static constexpr double
ToNormalisedHours(FloatDuration t) noexcept
{
  return t.count() > 0
    ? ToHours(t)
    : 0.;
}

void
FlightStatistics::Reset() noexcept
{
  const std::lock_guard lock{mutex};

  thermal_average.Reset();
  altitude.Reset();
  altitude_base.Reset();
  altitude_ceiling.Reset();
  task_speed.Reset();
  altitude_terrain.Reset();
  vario_circling_histogram.Reset(-7.5,7.5);
  vario_cruise_histogram.Reset(-7.5,7.5);
}

void
FlightStatistics::StartTask() noexcept
{
  const std::lock_guard lock{mutex};
  // JMW clear thermal climb average on task start
  //  thermal_average.Reset();
  vario_circling_histogram.Clear();
  vario_cruise_histogram.Clear();
}

void
FlightStatistics::AddAltitudeTerrain(const FloatDuration tflight,
                                     const double terrainalt) noexcept
{
  const std::lock_guard lock{mutex};
  altitude_terrain.Update(ToNormalisedHours(tflight), terrainalt);
}

void
FlightStatistics::AddAltitude(const FloatDuration tflight,
                              const double alt, const bool final_glide) noexcept
{
  const double t = ToNormalisedHours(tflight);

  const std::lock_guard lock{mutex};

  altitude.Update(t, alt);

  // update working ceiling immediately if above
  if (!altitude_ceiling.IsEmpty() && (alt > altitude_ceiling.GetLastY()))
    altitude_ceiling.UpdateConvexPositive(t, alt);

  // update working base immediately if below and not in final glide
  if (!altitude_base.IsEmpty() && (alt < altitude_base.GetLastY()) && !final_glide)
    altitude_base.UpdateConvexNegative(t, alt);
}

double
FlightStatistics::AverageThermalAdjusted(const double mc_current,
                                         const bool circling) noexcept
{
  const std::lock_guard lock{mutex};

  double mc_stats;
  if (! thermal_average.IsEmpty() && (thermal_average.GetAverageY() > 0)) {
    if (mc_current > 0 && circling)
      mc_stats = (thermal_average.GetCount() * thermal_average.GetAverageY() + mc_current) /
        (thermal_average.GetCount() + 1);
    else
      mc_stats = thermal_average.GetAverageY();
  } else {
    mc_stats = mc_current;
  }

  return mc_stats;
}

void
FlightStatistics::AddTaskSpeed(const FloatDuration tflight,
                               const double val) noexcept
{
  const std::lock_guard lock{mutex};
  task_speed.Update(ToHours(tflight), val);
}

void
FlightStatistics::AddClimbBase(const FloatDuration tflight,
                               const double alt) noexcept
{
  const std::lock_guard lock{mutex};

  // only add base after finished second climb, to avoid having the takeoff height
  // as the base
  //
  if (altitude_ceiling.HasResult())
    altitude_base.UpdateConvexNegative(ToNormalisedHours(tflight), alt);
}

void
FlightStatistics::AddClimbCeiling(const FloatDuration tflight,
                                  const double alt) noexcept
{
  const std::lock_guard lock{mutex};
  altitude_ceiling.UpdateConvexPositive(ToNormalisedHours(tflight), alt);
}

/**
 * Adds a thermal to the ThermalAverage calculator
 * @param v Average climb speed of the last thermal
 */
void
FlightStatistics::AddThermalAverage(const FloatDuration tflight_start,
                                    const FloatDuration tflight_end,
                                    const double v) noexcept
{
  const std::lock_guard lock{mutex};
  thermal_average.Update(ToNormalisedHours(tflight_start), v,
                         ToHours(tflight_end - tflight_start));
}

void
FlightStatistics::AddClimbRate([[maybe_unused]] const FloatDuration tflight,
                               const double vario, const bool circling) noexcept
{
  const std::lock_guard lock{mutex};
  if (circling) {
    vario_circling_histogram.UpdateHistogram(vario);
  } else {
    vario_cruise_histogram.UpdateHistogram(vario);
  }
}

double
FlightStatistics::GetMinWorkingHeight() const noexcept
{
  if (altitude_base.IsEmpty())
    return 0;

  // working height is average base less one standard deviation, or
  // the minimum encountered if this is higher
  return std::max(altitude_base.GetMinY(), altitude_base.GetAverageY() - sqrt(altitude_base.GetVarY()));
}

double
FlightStatistics::GetMaxWorkingHeight() const noexcept
{
  if (altitude_ceiling.IsEmpty())
    return 0;

  // working height is average ceiling plus one standard deviation, or
  // the maximum encountered if this is lower
  double result = std::min(altitude_ceiling.GetMaxY(),
                           altitude_ceiling.GetAverageY() + sqrt(altitude_ceiling.GetVarY()));
  if (!altitude.IsEmpty())
    result = std::max(altitude.GetMaxY(), result);

  return result;
}

// percentile to look up to determine max/min value
static constexpr double PERCENTILE_VARIO = 0.1;

double
FlightStatistics::GetVarioScalePositive() const noexcept
{
  return std::max(vario_circling_histogram.GetPercentile(1-PERCENTILE_VARIO),
                  vario_cruise_histogram.GetPercentile(1-PERCENTILE_VARIO));
}

double
FlightStatistics::GetVarioScaleNegative() const noexcept
{
  return std::min(vario_circling_histogram.GetPercentile(PERCENTILE_VARIO),
                  vario_cruise_histogram.GetPercentile(PERCENTILE_VARIO));
}
