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
ComputePressure(NMEA_INFO &basic, const SETTINGS_COMPUTER &settings_computer)
{
  const AtmosphericPressure &qnh = settings_computer.pressure;
  const bool qnh_available = settings_computer.pressure_available;
  const bool static_pressure_available = basic.static_pressure_available;

  if (!basic.static_pressure_available) {
    if (basic.PressureAltitudeAvailable) {
      basic.static_pressure =
        AtmosphericPressure::PressureAltitudeToStaticPressure(basic.PressureAltitude);
      basic.static_pressure_available = basic.PressureAltitudeAvailable;
    } else if (basic.BaroAltitudeAvailable && qnh_available)
      basic.static_pressure =
        qnh.QNHAltitudeToStaticPressure(basic.BaroAltitude);
  }

  if (!basic.PressureAltitudeAvailable) {
    if (basic.static_pressure_available) {
      basic.PressureAltitude =
        AtmosphericPressure::StaticPressureToPressureAltitude(basic.static_pressure);
      basic.PressureAltitudeAvailable = basic.static_pressure_available;
    } else if (basic.BaroAltitudeAvailable && qnh_available)
      basic.PressureAltitude =
        qnh.QNHAltitudeToPressureAltitude(basic.BaroAltitude);
  }

  if (qnh_available) {
    /* if the current pressure and the QNH is known, then true baro
       altitude should be discarded, because the external device which
       provided it may have a different QNH setting */

    if (static_pressure_available) {
      basic.BaroAltitude =
        qnh.StaticPressureToQNHAltitude(basic.static_pressure);
      basic.BaroAltitudeAvailable = basic.static_pressure_available;
    } else if (basic.PressureAltitudeAvailable) {
      basic.BaroAltitude =
        qnh.PressureAltitudeToQNHAltitude(basic.PressureAltitude);
      basic.BaroAltitudeAvailable = basic.PressureAltitudeAvailable;
    }
  } else if (!basic.BaroAltitudeAvailable && basic.PressureAltitudeAvailable)
    /* no QNH, but let's fill in the best fallback value we can get,
       without setting BaroAltitudeAvailable */
    basic.BaroAltitude = basic.PressureAltitude;
}

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
  ComputePressure(data, settings_computer);
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
