/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#define k1 fixed(0.190263)
#define inv_k1 fixed(1.0 / 0.190263)
#define k2 fixed(8.417286e-5)
#define inv_k2 fixed(1.0 / 8.417286e-5)

void
AtmosphericPressure::Adjust(const fixed factor,
                            const fixed offset)
{
  value = value * factor + offset;
}

AtmosphericPressure
AtmosphericPressure::QNHAltitudeToStaticPressure(const fixed alt) const
{
  return HectoPascal(pow((pow(GetHectoPascal(), k1) - k2 * alt), inv_k1));
}

AtmosphericPressure
AtmosphericPressure::PressureAltitudeToStaticPressure(const fixed alt)
{
  return Standard().QNHAltitudeToStaticPressure(alt);
}


fixed
AtmosphericPressure::StaticPressureToQNHAltitude(const AtmosphericPressure ps) const
{
  return (pow(GetHectoPascal(), k1) - pow(ps.GetHectoPascal(), k1)) * inv_k2;
}

fixed
AtmosphericPressure::PressureAltitudeToQNHAltitude(const fixed alt) const
{
  return StaticPressureToQNHAltitude(PressureAltitudeToStaticPressure(alt));
}

fixed
AtmosphericPressure::QNHAltitudeToPressureAltitude(const fixed alt) const
{
  return StaticPressureToPressureAltitude(QNHAltitudeToStaticPressure(alt));
}

fixed
AtmosphericPressure::StaticPressureToPressureAltitude(const AtmosphericPressure ps)
{
  return Standard().StaticPressureToQNHAltitude(ps);
}

AtmosphericPressure
AtmosphericPressure::FindQNHFromPressure(const AtmosphericPressure pressure,
                                         const fixed alt_known)
{
  return pressure.QNHAltitudeToStaticPressure(-alt_known);
}
