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
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "SettingsComputer.hpp"

#define fixed_inv_g fixed(1.0/9.81)
#define fixed_inv_2g fixed(1.0/(2.0*9.81))
#define fixed_small fixed(0.001)

/**
 * Fill vario values when they are provided by the external vario.
 * This is a short path that works even when no GPS (providing GPS
 * time) is connected.
 */
static void
FillVario(MoreData &data)
{
  if (data.TotalEnergyVarioAvailable) {
    data.BruttoVario = data.TotalEnergyVario;

    if (!data.NettoVarioAvailable)
      /* copy the NettoVario value from BruttoVario; it will be
         overwritten by ComputeNettoVario() if the GliderSinkRate is
         known */
      data.NettoVario = data.BruttoVario;
  }
}

static void
ComputePressure(NMEA_INFO &basic, const SETTINGS_COMPUTER &settings_computer)
{
  const AtmosphericPressure &qnh = settings_computer.pressure;
  const bool qnh_available = settings_computer.pressure_available;
  const bool static_pressure_available = basic.static_pressure_available;
  const bool pressure_altitude_available = basic.PressureAltitudeAvailable;

  if (!basic.static_pressure_available) {
    if (basic.PressureAltitudeAvailable) {
      basic.static_pressure =
        AtmosphericPressure::PressureAltitudeToStaticPressure(basic.PressureAltitude);
      basic.static_pressure_available = basic.PressureAltitudeAvailable;
    } else if (basic.BaroAltitudeAvailable && qnh_available) {
      basic.static_pressure =
        qnh.QNHAltitudeToStaticPressure(basic.BaroAltitude);
      basic.static_pressure_available = basic.BaroAltitudeAvailable;
    }
  }

  if (!basic.PressureAltitudeAvailable) {
    if (static_pressure_available) {
      basic.PressureAltitude =
        AtmosphericPressure::StaticPressureToPressureAltitude(basic.static_pressure);
      basic.PressureAltitudeAvailable = basic.static_pressure_available;
    } else if (basic.BaroAltitudeAvailable && qnh_available) {
      basic.PressureAltitude =
        qnh.QNHAltitudeToPressureAltitude(basic.BaroAltitude);
      basic.PressureAltitudeAvailable = basic.BaroAltitudeAvailable;
    }
  }

  if (qnh_available) {
    /* if the current pressure and the QNH is known, then true baro
       altitude should be discarded, because the external device which
       provided it may have a different QNH setting */

    if (static_pressure_available) {
      basic.BaroAltitude =
        qnh.StaticPressureToQNHAltitude(basic.static_pressure);
      basic.BaroAltitudeAvailable = basic.static_pressure_available;
    } else if (pressure_altitude_available) {
      basic.BaroAltitude =
        qnh.PressureAltitudeToQNHAltitude(basic.PressureAltitude);
      basic.BaroAltitudeAvailable = basic.PressureAltitudeAvailable;
    }
  } else if (!basic.BaroAltitudeAvailable && pressure_altitude_available)
    /* no QNH, but let's fill in the best fallback value we can get,
       without setting BaroAltitudeAvailable */
    basic.BaroAltitude = basic.PressureAltitude;
}

static void
ComputeNavAltitude(MoreData &basic,
                   const SETTINGS_COMPUTER &settings_computer)
{
  basic.NavAltitude = settings_computer.EnableNavBaroAltitude &&
    basic.BaroAltitudeAvailable
    ? basic.BaroAltitude
    : basic.GPSAltitude;
}

static void
ComputeTrack(NMEA_INFO &basic, const NMEA_INFO &last)
{
  if (basic.track_available || !basic.LocationAvailable ||
      !last.LocationAvailable)
    return;

  const GeoVector v = last.Location.distance_bearing(basic.Location);
  if (v.Distance >= fixed_one)
    basic.track = v.Bearing;
}

static void
ComputeGroundSpeed(NMEA_INFO &basic, const NMEA_INFO &last)
{
  assert(basic.time_available);
  assert(last.time_available);
  assert(basic.Time > last.Time);

  if (basic.GroundSpeedAvailable)
    return;

  basic.GroundSpeed = fixed_zero;
  if (!basic.LocationAvailable || !last.LocationAvailable)
    return;

  fixed t = basic.Time - last.Time;
  fixed d = basic.Location.distance(last.Location);
  basic.GroundSpeed = d / t;
}

/**
 * Attempt to compute airspeed from ground speed and wind if it's not
 * available.
 */
static void
ComputeAirspeed(NMEA_INFO &basic, const DERIVED_INFO &calculated)
{
  if (basic.AirspeedAvailable && basic.AirspeedReal)
    /* got it already */
    return;

  if (!basic.GroundSpeedAvailable || !calculated.wind_available ||
      !calculated.flight.Flying) {
    /* impossible to calculate */
    basic.AirspeedAvailable.Clear();
    return;
  }

  fixed TrueAirspeedEstimated = fixed_zero;

  const SpeedVector wind = calculated.wind;
  if (positive(basic.GroundSpeed) || wind.is_non_zero()) {
    fixed x0 = basic.track.fastsine() * basic.GroundSpeed;
    fixed y0 = basic.track.fastcosine() * basic.GroundSpeed;
    x0 += wind.bearing.fastsine() * wind.norm;
    y0 += wind.bearing.fastcosine() * wind.norm;

    TrueAirspeedEstimated = hypot(x0, y0);
  }

  basic.TrueAirspeed = TrueAirspeedEstimated;
  basic.IndicatedAirspeed = TrueAirspeedEstimated
    / AtmosphericPressure::AirDensityRatio(basic.GetAltitudeBaroPreferred());
  basic.AirspeedAvailable.Update(basic.clock);
  basic.AirspeedReal = false;
}

/**
 * Calculates energy height on TAS basis
 *
 * \f${m/2} \times v^2 = m \times g \times h\f$ therefore \f$h = {v^2}/{2 \times g}\f$
 */
static void
ComputeEnergyHeight(MoreData &basic)
{
  if (basic.AirspeedAvailable)
    basic.EnergyHeight = sqr(basic.TrueAirspeed) * fixed_inv_2g;
  else
    /* setting EnergyHeight to zero is the safe approach, as we don't know the kinetic energy
       of the glider for sure. */
    basic.EnergyHeight = fixed_zero;

  basic.TEAltitude = basic.NavAltitude + basic.EnergyHeight;
}

/**
 * Calculates the vario values for gps vario, gps total energy vario
 * Sets Vario to GPSVario or received Vario data from instrument
 */
static void
ComputeGPSVario(MoreData &basic, const MoreData &last)
{
  assert(basic.time_available);
  assert(last.time_available);
  assert(basic.Time > last.Time);

  // Calculate time passed since last calculation
  const fixed dT = basic.Time - last.Time;

  const fixed Gain = basic.NavAltitude - last.NavAltitude;
  const fixed GainTE = basic.TEAltitude - last.TEAltitude;

  // estimate value from GPS
  basic.GPSVario = Gain / dT;
  basic.GPSVarioTE = GainTE / dT;
}

static void
ComputeBruttoVario(MoreData &basic)
{
  basic.BruttoVario = basic.TotalEnergyVarioAvailable
    ? basic.TotalEnergyVario
    : basic.GPSVario;
}

/**
 * Compute the NettoVario value if it's unavailable.
 */
static void
ComputeNettoVario(MoreData &basic, const VARIO_INFO &vario)
{
  if (basic.NettoVarioAvailable)
    /* got it already */
    return;

  basic.NettoVario = basic.BruttoVario - vario.GliderSinkRate;
}

/**
 * Calculates the turn rate of the heading,
 * the estimated bank angle and
 * the estimated pitch angle
 */
static void
ComputeDynamics(MoreData &basic, const DERIVED_INFO &calculated)
{
  if (calculated.flight.Flying &&
      (positive(basic.GroundSpeed) || calculated.wind.is_non_zero())) {

    // estimate bank angle (assuming balanced turn)
    if (basic.AirspeedAvailable) {
      const fixed angle = atan(Angle::degrees(calculated.TurnRateWind
          * basic.TrueAirspeed * fixed_inv_g).value_radians());

      basic.acceleration.BankAngle = Angle::radians(angle);
      if (!basic.acceleration.Available)
        basic.acceleration.Gload = fixed_one / max(fixed_small, fabs(cos(angle)));
    } else {
      basic.acceleration.BankAngle = Angle::native(fixed_zero);
      if (!basic.acceleration.Available)
        basic.acceleration.Gload = fixed_one;
    }

    // estimate pitch angle (assuming balanced turn)
    if (basic.AirspeedAvailable && basic.TotalEnergyVarioAvailable)
      basic.acceleration.PitchAngle = Angle::radians(atan2(basic.GPSVario - basic.TotalEnergyVario,
                                                           basic.TrueAirspeed));
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
BasicComputer::Fill(MoreData &data, const SETTINGS_COMPUTER &settings_computer)
{
  FillVario(data);
  ComputePressure(data, settings_computer);
  ComputeNavAltitude(data, settings_computer);
}

void
BasicComputer::Compute(MoreData &data, const MoreData &last,
                       const DERIVED_INFO &calculated,
                       const SETTINGS_COMPUTER &settings_computer)
{
  if (!data.time_available || !last.time_available ||
      data.Time <= last.Time)
    return;

  ComputeTrack(data, last);
  ComputeGroundSpeed(data, last);
  ComputeAirspeed(data, calculated);
  ComputeEnergyHeight(data);
  ComputeGPSVario(data, last);
  ComputeBruttoVario(data);
  ComputeNettoVario(data, calculated);
  ComputeDynamics(data, calculated);
}
