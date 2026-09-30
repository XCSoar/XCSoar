// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalProjection.hpp"

#include "Geo/GeoPoint.hpp"
#include "Geo/Math.hpp"
#include "Geo/SpeedVector.hpp"

#include <cmath>

#if defined(__GNUC__) && !defined(__clang__)
/* GCC's -ffast-math makes isfinite() assume that all inputs are finite. */
#pragma GCC push_options
#pragma GCC optimize ("no-finite-math-only")
#endif

SpeedVector
CalculateThermalDriftPerMeter(const SpeedVector &wind,
                              double lift_rate) noexcept
{
  if (!(lift_rate > 0) || !std::isfinite(lift_rate) ||
      !(wind.norm > 0) || !std::isfinite(wind.norm) ||
      !std::isfinite(wind.bearing.Native()))
    return SpeedVector::Zero();

  const double norm = wind.norm / lift_rate;
  return norm > 0 && std::isfinite(norm)
    ? SpeedVector(wind.bearing, norm)
    : SpeedVector::Zero();
}

GeoPoint
ProjectThermalCore(const GeoPoint &location, double height_delta,
                   const SpeedVector &drift_per_meter) noexcept
{
  if (!location.IsValid() || height_delta == 0 ||
      !std::isfinite(height_delta) ||
      !(drift_per_meter.norm > 0) ||
      !std::isfinite(drift_per_meter.norm) ||
      !std::isfinite(drift_per_meter.bearing.Native()))
    return location;

  const Angle bearing = height_delta > 0
    ? drift_per_meter.bearing.Reciprocal()
    : drift_per_meter.bearing;
  return FindLatitudeLongitude(location, bearing,
                               drift_per_meter.norm *
                               std::abs(height_delta));
}

GeoPoint
ProjectThermalCore(const GeoPoint &location, double height_delta,
                   const SpeedVector &wind, double lift_rate) noexcept
{
  return ProjectThermalCore(location, height_delta,
                           CalculateThermalDriftPerMeter(wind, lift_rate));
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
