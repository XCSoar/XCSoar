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

#include "BasicComputer.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "SettingsComputer.hpp"

#define fixed_inv_g fixed(1.0/9.81)
#define fixed_small fixed(0.001)

static void
ComputeNavAltitude(NMEA_INFO &basic,
                   const SETTINGS_COMPUTER &settings_computer)
{
  basic.NavAltitude = settings_computer.EnableNavBaroAltitude &&
    basic.BaroAltitudeAvailable
    ? basic.BaroAltitude
    : basic.GPSAltitude;
}

/**
 * Calculates the turn rate of the heading,
 * the estimated bank angle and
 * the estimated pitch angle
 */
static void
ComputeDynamics(NMEA_INFO &basic, const DERIVED_INFO &calculated)
{
  if (calculated.flight.Flying &&
      (positive(basic.GroundSpeed) || calculated.wind.is_non_zero())) {

    // estimate bank angle (assuming balanced turn)
    if (calculated.AirspeedAvailable) {
      const fixed angle = atan(Angle::degrees(calculated.TurnRateWind
          * calculated.TrueAirspeed * fixed_inv_g).value_radians());

      basic.acceleration.BankAngle = Angle::radians(angle);
      if (!basic.acceleration.Available)
        basic.acceleration.Gload = fixed_one / max(fixed_small, fabs(cos(angle)));
    } else {
      basic.acceleration.BankAngle = Angle::native(fixed_zero);
      if (!basic.acceleration.Available)
        basic.acceleration.Gload = fixed_one;
    }

    // estimate pitch angle (assuming balanced turn)
    if (calculated.AirspeedAvailable && basic.TotalEnergyVarioAvailable)
      basic.acceleration.PitchAngle = Angle::radians(atan2(calculated.GPSVario - basic.TotalEnergyVario,
          calculated.TrueAirspeed));
    else
      basic.acceleration.PitchAngle = Angle::native(fixed_zero);

  } else {
    basic.acceleration.BankAngle = Angle::native(fixed_zero);
    basic.acceleration.PitchAngle = Angle::native(fixed_zero);

    if (!basic.acceleration.Available)
      basic.acceleration.Gload = fixed_one;
  }
}

void
BasicComputer::Fill(NMEA_INFO &data, const SETTINGS_COMPUTER &settings_computer)
{
  ComputeNavAltitude(data, settings_computer);
}

void
BasicComputer::Compute(NMEA_INFO &data, const NMEA_INFO &last,
                       const DERIVED_INFO &calculated,
                       const SETTINGS_COMPUTER &settings_computer)
{
  if (data.Time > last.Time)
    ComputeDynamics(data, calculated);
}
