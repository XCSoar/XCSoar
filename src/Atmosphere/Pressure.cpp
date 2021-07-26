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
