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
  if (data.total_energy_vario_available) {
    data.brutto_vario = data.total_energy_vario;

    if (!data.netto_vario_available)
      /* copy the NettoVario value from BruttoVario; it will be
         overwritten by ComputeNettoVario() if the GliderSinkRate is
         known */
      data.netto_vario = data.brutto_vario;
  }
}

static void
ComputePressure(NMEAInfo &basic, const SETTINGS_COMPUTER &settings_computer)
{
  const AtmosphericPressure &qnh = settings_computer.pressure;
  const bool qnh_available = settings_computer.pressure_available;
  const bool static_pressure_available = basic.static_pressure_available;
  const bool pressure_altitude_available = basic.pressure_altitude_available;

  if (!basic.static_pressure_available) {
    if (basic.pressure_altitude_available) {
      basic.static_pressure =
        AtmosphericPressure::PressureAltitudeToStaticPressure(basic.pressure_altitude);
      basic.static_pressure_available = basic.pressure_altitude_available;
    } else if (basic.baro_altitude_available && qnh_available) {
      basic.static_pressure =
        qnh.QNHAltitudeToStaticPressure(basic.baro_altitude);
      basic.static_pressure_available = basic.baro_altitude_available;
    }
  }

  if (!basic.pressure_altitude_available) {
    if (static_pressure_available) {
      basic.pressure_altitude =
        AtmosphericPressure::StaticPressureToPressureAltitude(basic.static_pressure);
      basic.pressure_altitude_available = basic.static_pressure_available;
    } else if (basic.baro_altitude_available && qnh_available) {
      basic.pressure_altitude =
        qnh.QNHAltitudeToPressureAltitude(basic.baro_altitude);
      basic.pressure_altitude_available = basic.baro_altitude_available;
    }
  }

  if (qnh_available) {
    /* if the current pressure and the QNH is known, then true baro
       altitude should be discarded, because the external device which
       provided it may have a different QNH setting */

    if (static_pressure_available) {
      basic.baro_altitude =
        qnh.StaticPressureToQNHAltitude(basic.static_pressure);
      basic.baro_altitude_available = basic.static_pressure_available;
    } else if (pressure_altitude_available) {
      basic.baro_altitude =
        qnh.PressureAltitudeToQNHAltitude(basic.pressure_altitude);
      basic.baro_altitude_available = basic.pressure_altitude_available;
    }
  } else if (!basic.baro_altitude_available && pressure_altitude_available)
    /* no QNH, but let's fill in the best fallback value we can get,
       without setting BaroAltitudeAvailable */
    basic.baro_altitude = basic.pressure_altitude;
}

static void
ComputeNavAltitude(MoreData &basic,
                   const SETTINGS_COMPUTER &settings_computer)
{
  basic.nav_altitude = settings_computer.nav_baro_altitude_enabled &&
    basic.baro_altitude_available
    ? basic.baro_altitude
    : basic.gps_altitude;
}

static void
ComputeTrack(NMEAInfo &basic, const NMEAInfo &last)
{
  if (basic.track_available || !basic.location_available ||
      !last.location_available)
    return;

  const GeoVector v = last.location.DistanceBearing(basic.location);
  if (v.distance >= fixed_one)
    basic.track = v.bearing;
}

static void
ComputeGroundSpeed(NMEAInfo &basic, const NMEAInfo &last)
{
  assert(basic.time_available);
  assert(last.time_available);
  assert(basic.time > last.time);

  if (basic.ground_speed_available)
    return;

  basic.ground_speed = fixed_zero;
  if (!basic.location_available || !last.location_available)
    return;

  fixed t = basic.time - last.time;
  fixed d = basic.location.Distance(last.location);
  basic.ground_speed = d / t;
}

/**
 * Attempt to compute airspeed from ground speed and wind if it's not
 * available.
 */
static void
ComputeAirspeed(NMEAInfo &basic, const DerivedInfo &calculated)
{
  if (basic.airspeed_available && basic.airspeed_real)
    /* got it already */
    return;

  if (!basic.ground_speed_available || !calculated.wind_available ||
      !calculated.flight.flying) {
    /* impossible to calculate */
    basic.airspeed_available.Clear();
    return;
  }

  fixed TrueAirspeedEstimated = fixed_zero;

  const SpeedVector wind = calculated.wind;
  if (positive(basic.ground_speed) || wind.is_non_zero()) {
    fixed x0 = basic.track.fastsine() * basic.ground_speed;
    fixed y0 = basic.track.fastcosine() * basic.ground_speed;
    x0 += wind.bearing.fastsine() * wind.norm;
    y0 += wind.bearing.fastcosine() * wind.norm;

    TrueAirspeedEstimated = hypot(x0, y0);
  }

  basic.true_airspeed = TrueAirspeedEstimated;
  basic.indicated_airspeed = TrueAirspeedEstimated
    / AtmosphericPressure::AirDensityRatio(basic.GetAltitudeBaroPreferred());
  basic.airspeed_available.Update(basic.clock);
  basic.airspeed_real = false;
}

/**
 * Calculates energy height on TAS basis
 *
 * \f${m/2} \times v^2 = m \times g \times h\f$ therefore \f$h = {v^2}/{2 \times g}\f$
 */
static void
ComputeEnergyHeight(MoreData &basic)
{
  if (basic.airspeed_available)
    basic.energy_height = sqr(basic.true_airspeed) * fixed_inv_2g;
  else
    /* setting EnergyHeight to zero is the safe approach, as we don't know the kinetic energy
       of the glider for sure. */
    basic.energy_height = fixed_zero;

  basic.TE_altitude = basic.nav_altitude + basic.energy_height;
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
  assert(basic.time > last.time);

  if (!basic.NavAltitudeAvailable() || !last.NavAltitudeAvailable()) {
    basic.gps_vario = basic.gps_vario_TE = fixed_zero;
    return;
  }

  // Calculate time passed since last calculation
  const fixed dT = basic.time - last.time;

  const fixed Gain = basic.nav_altitude - last.nav_altitude;
  const fixed GainTE = basic.TE_altitude - last.TE_altitude;

  // estimate value from GPS
  basic.gps_vario = Gain / dT;
  basic.gps_vario_TE = GainTE / dT;
}

static void
ComputeBruttoVario(MoreData &basic)
{
  basic.brutto_vario = basic.total_energy_vario_available
    ? basic.total_energy_vario
    : basic.gps_vario;
}

/**
 * Compute the NettoVario value if it's unavailable.
 */
static void
ComputeNettoVario(MoreData &basic, const VarioInfo &vario)
{
  if (basic.netto_vario_available)
    /* got it already */
    return;

  basic.netto_vario = basic.brutto_vario - vario.sink_rate;
}

/**
 * Calculates the turn rate of the heading,
 * the estimated bank angle and
 * the estimated pitch angle
 */
static void
ComputeDynamics(MoreData &basic, const DerivedInfo &calculated)
{
  if (calculated.flight.flying &&
      (positive(basic.ground_speed) ||
       (calculated.wind_available && calculated.wind.is_non_zero()))) {

    // estimate bank angle (assuming balanced turn)
    if (basic.airspeed_available) {
      const fixed angle = atan(Angle::Degrees(calculated.turn_rate_heading
          * basic.true_airspeed * fixed_inv_g).Radians());

      basic.acceleration.bank_angle = Angle::Radians(angle);
      if (!basic.acceleration.available)
        basic.acceleration.g_load = fixed_one / max(fixed_small, fabs(cos(angle)));
    } else {
      basic.acceleration.bank_angle = Angle::Zero();
      if (!basic.acceleration.available)
        basic.acceleration.g_load = fixed_one;
    }

    // estimate pitch angle (assuming balanced turn)
    if (basic.airspeed_available && basic.total_energy_vario_available)
      basic.acceleration.pitch_angle = Angle::Radians(atan2(basic.gps_vario - basic.total_energy_vario,
                                                           basic.true_airspeed));
    else
      basic.acceleration.pitch_angle = Angle::Zero();

  } else {
    basic.acceleration.bank_angle = Angle::Zero();
    basic.acceleration.pitch_angle = Angle::Zero();

    if (!basic.acceleration.available)
      basic.acceleration.g_load = fixed_one;
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
                       const DerivedInfo &calculated,
                       const SETTINGS_COMPUTER &settings_computer)
{
  if (!data.HasTimeAdvancedSince(last))
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
