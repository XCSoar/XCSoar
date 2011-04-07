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

#include "Pressure.hpp"

#define fixed_101325 fixed(1013.25)
#define isa_sea_level_density fixed(1.225)
#define hpa_to_pa fixed_int_constant(100)
#define pa_to_hpa fixed(0.01)
#define k1 fixed(0.190263)
#define inv_k1 fixed(1.0 / 0.190263)
#define k2 fixed(8.417286e-5)
#define inv_k2 fixed(1.0 / 8.417286e-5)
#define k4 fixed(44330.8)
#define k5 fixed(1.0 / 4946.54)
#define k6 fixed(1.0 / 42266.5)
#define k7 fixed(1.0 / 0.234969)

AtmosphericPressure::AtmosphericPressure():
  m_QNH(fixed_101325) {}

fixed
AtmosphericPressure::QNHAltitudeToStaticPressure(const fixed alt) const
{
  return hpa_to_pa * pow((pow(m_QNH, k1) - k2 * alt), inv_k1);
}

fixed
AtmosphericPressure::PressureAltitudeToStaticPressure(const fixed alt)
{
  return hpa_to_pa * pow((pow(fixed_101325, k1) - k2 * alt), inv_k1);
}


fixed
AtmosphericPressure::StaticPressureToQNHAltitude(const fixed ps) const
{
  return (pow(m_QNH, k1) - pow(ps * pa_to_hpa, k1)) * inv_k2;
}

fixed
AtmosphericPressure::PressureAltitudeToQNHAltitude(const fixed alt) const
{
  return StaticPressureToQNHAltitude(fixed(pow((k4 - alt) * k5, inv_k1)));
}

fixed
AtmosphericPressure::QNHAltitudeToPressureAltitude(const fixed alt) const
{
  return (pow(fixed_101325, k1) - pow(QNHAltitudeToStaticPressure(alt) * pa_to_hpa, k1)) * inv_k2;
}

fixed
AtmosphericPressure::StaticPressureToPressureAltitude(const fixed ps)
{
  return (pow(fixed_101325, k1) - pow(ps * pa_to_hpa, k1)) * inv_k2;
}

fixed
AtmosphericPressure::FindQNHFromPressureAltitude(const fixed alt_raw,
                                                 const fixed alt_known) const
{
  // step 1, find static pressure from device assuming it's QNH adjusted
  const fixed psraw = QNHAltitudeToStaticPressure(alt_raw);

  // step 2, calculate QNH so that reported alt will be known alt
  return pow(pow(psraw * pa_to_hpa, k1) + k2 * alt_known, inv_k1);
}

fixed
AtmosphericPressure::AirDensity(const fixed altitude)
{
  return fixed(pow((k4 - altitude) * k6, k7));
}

fixed
AtmosphericPressure::AirDensityRatio(const fixed altitude)
{
  return sqrt(isa_sea_level_density / AirDensity(altitude));
}

