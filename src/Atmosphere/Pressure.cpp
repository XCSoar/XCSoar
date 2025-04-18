// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Pressure.hpp"

#include <math.h>

static constexpr double k1 = 0.190263;
static constexpr double inv_k1 = 1.0 / k1;
static constexpr double k2 = 8.417286e-5;
static constexpr double inv_k2 = 1.0 / k2;

AtmosphericPressure
AtmosphericPressure::QNHAltitudeToStaticPressure(const double alt) const noexcept
{
  return HectoPascal(pow((pow(GetHectoPascal(), k1) - k2 * alt), inv_k1));
}

AtmosphericPressure
AtmosphericPressure::PressureAltitudeToStaticPressure(const double alt) noexcept
{
  return Standard().QNHAltitudeToStaticPressure(alt);
}


double
AtmosphericPressure::StaticPressureToQNHAltitude(const AtmosphericPressure ps) const noexcept
{
  return (pow(GetHectoPascal(), k1) - pow(ps.GetHectoPascal(), k1)) * inv_k2;
}

double
AtmosphericPressure::PressureAltitudeToQNHAltitude(const double alt) const noexcept
{
  return StaticPressureToQNHAltitude(PressureAltitudeToStaticPressure(alt));
}

double
AtmosphericPressure::QNHAltitudeToPressureAltitude(const double alt) const noexcept
{
  return StaticPressureToPressureAltitude(QNHAltitudeToStaticPressure(alt));
}

double
AtmosphericPressure::StaticPressureToPressureAltitude(const AtmosphericPressure ps) noexcept
{
  return Standard().StaticPressureToQNHAltitude(ps);
}

AtmosphericPressure
AtmosphericPressure::FindQNHFromPressure(const AtmosphericPressure pressure,
                                         const double alt_known) noexcept
{
  return pressure.QNHAltitudeToStaticPressure(-alt_known);
}
