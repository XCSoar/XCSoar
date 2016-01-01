/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Settings.hpp"
#include "Atmosphere/AirDensity.hpp"
#include "Geo/Gravity.hpp"
#include "Math/Util.hpp"

static constexpr double INVERSE_G = 1. / GRAVITY;
static constexpr double INVERSE_2G = INVERSE_G / 2.;

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
    data.brutto_vario_available = data.total_energy_vario_available;

    if (!data.netto_vario_available)
      /* copy the NettoVario value from BruttoVario; it will be
         overwritten by ComputeNettoVario() if the GliderSinkRate is
         known */
      data.netto_vario = data.brutto_vario;
  }
}

static void
ComputePressure(NMEAInfo &basic, const AtmosphericPressure qnh)
{
  const bool qnh_available = qnh.IsPlausible();
  const bool static_pressure_available = basic.static_pressure_available;
  const bool pressure_altitude_available = basic.pressure_altitude_available;

  if (!static_pressure_available) {
    if (pressure_altitude_available) {
      basic.static_pressure =
        AtmosphericPressure::PressureAltitudeToStaticPressure(basic.pressure_altitude);
      basic.static_pressure_available = basic.pressure_altitude_available;
    } else if (basic.baro_altitude_available && qnh_available) {
      basic.static_pressure =
        qnh.QNHAltitudeToStaticPressure(basic.baro_altitude);
      basic.static_pressure_available = basic.baro_altitude_available;
    }
  }

  if (!pressure_altitude_available) {
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
ComputeNavAltitude(MoreData &basic, const FeaturesSettings &features)
{
  basic.nav_altitude = features.nav_baro_altitude_enabled &&
    basic.baro_altitude_available
    ? basic.baro_altitude
    : basic.gps_altitude;
}

static void
ComputeTrack(NMEAInfo &basic, const NMEAInfo &last)
{
  if (basic.track_available ||
      !basic.location_available ||
      !last.location_available ||
      !basic.location_available.Modified(last.location_available))
    return;

  const GeoVector v = last.location.DistanceBearing(basic.location);
  if (v.distance >= 1) {
    basic.track = v.bearing;
    basic.track_available = basic.location_available;
  }
}

/**
 * Fallback heading calculation if no compass is connected.
 */
static void
ComputeHeading(AttitudeState &attitude, const NMEAInfo &basic,
               const DerivedInfo &calculated)
{
  if (attitude.heading_available) {
    /* compass connected, don't need to calculate it */
    attitude.heading_computed = false;
    return;
  }

  if (!basic.track_available) {
    /* calculation not possible; set a dummy value (heading north) to
       avoid accessing uninitialised memory */
    attitude.heading = Angle::Zero();
    attitude.heading_computed = false;
    return;
  }

  if (basic.ground_speed_available && calculated.wind_available &&
      (basic.ground_speed > 0 || calculated.wind.IsNonZero()) &&
      calculated.flight.flying) {
    auto x0 = basic.track.fastsine() * basic.ground_speed;
    auto y0 = basic.track.fastcosine() * basic.ground_speed;
    x0 += calculated.wind.bearing.fastsine() * calculated.wind.norm;
    y0 += calculated.wind.bearing.fastcosine() * calculated.wind.norm;

    attitude.heading = Angle::FromXY(y0, x0).AsBearing();
  } else {
    attitude.heading = basic.track;
  }
  attitude.heading_computed = true;
}

/**
 * Attempt to compute airspeed when it is not yet available from:
 * 1) dynamic pressure and air density derived from some altitude.
 * 2) pitot pressure and static pressure.
 * 3) ground speed and wind.
 */
static void
ComputeAirspeed(NMEAInfo &basic, const DerivedInfo &calculated)
{
  if (basic.airspeed_available && basic.airspeed_real)
    /* got it already */
    return;

  const auto any_altitude = basic.GetAnyAltitude();

  if (!basic.airspeed_available && any_altitude.first) {
    double dyn; bool available = false;
    if (basic.dyn_pressure_available) {
      dyn = basic.dyn_pressure.GetHectoPascal();
      available = true;
    } else if (basic.pitot_pressure_available && basic.static_pressure_available) {
      dyn = basic.pitot_pressure.GetHectoPascal() - basic.static_pressure.GetHectoPascal();
      available = true;
    }
    if (available) {
      basic.indicated_airspeed = sqrt(double(163.2653061) * dyn);
      basic.true_airspeed = basic.indicated_airspeed *
                            AirDensityRatio(any_altitude.second);

      basic.airspeed_available.Update(basic.clock);
      basic.airspeed_real = true; // Anyway not less real then any other method.
      return;
    }
  }

  if (!basic.ground_speed_available || !calculated.wind_available ||
      !calculated.flight.flying) {
    /* impossible to calculate */
    basic.airspeed_available.Clear();
    return;
  }

  double TrueAirspeedEstimated = 0;

  const SpeedVector wind = calculated.wind;
  if (basic.ground_speed > 0 || wind.IsNonZero()) {
    auto x0 = basic.track.fastsine() * basic.ground_speed;
    auto y0 = basic.track.fastcosine() * basic.ground_speed;
    x0 += wind.bearing.fastsine() * wind.norm;
    y0 += wind.bearing.fastcosine() * wind.norm;

    TrueAirspeedEstimated = hypot(x0, y0);
  }

  basic.true_airspeed = TrueAirspeedEstimated;

  basic.indicated_airspeed = TrueAirspeedEstimated;
  if (any_altitude.first)
    basic.indicated_airspeed /= AirDensityRatio(any_altitude.second);

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
    basic.energy_height = Square(basic.true_airspeed) * INVERSE_2G;
  else
    /* setting EnergyHeight to zero is the safe approach, as we don't know the kinetic energy
       of the glider for sure. */
    basic.energy_height = 0;

  basic.TE_altitude = basic.nav_altitude + basic.energy_height;
}

/**
 * Calculates the vario values for gps vario, gps total energy vario
 * Sets Vario to GPSVario or received Vario data from instrument
 */
static void
ComputeGPSVario(MoreData &basic,
                const MoreData &last, const MoreData &last_gps)
{
  if (basic.noncomp_vario_available && last.noncomp_vario_available) {
    /* If we have a noncompensated vario signal, we use that to compute
       the "GPS" total energy vario value, even if navigate by GPS
       altitude is configured, because a vario is expected to be more
       exact. */

    /* use the "Validity" time stamp, because it reflects when this
       vertical speed was measured, and GPS time may not be available */
    const auto delta_t =
      basic.noncomp_vario_available.GetTimeDifference(last.noncomp_vario_available);

    if (delta_t > 0) {
      /* only update when a new value was received */

      auto delta_e = basic.energy_height - last.energy_height;

      basic.gps_vario = basic.noncomp_vario;
      basic.gps_vario_TE = basic.noncomp_vario + delta_e / delta_t;
      basic.gps_vario_available = basic.noncomp_vario_available;
    }
  } else if (basic.pressure_altitude_available && last.pressure_altitude_available) {
    /* Barring that, we prefer pressure altitude for the "GPS" vario,
       even if navigate by GPS altitude is configured, because pressure
       altitude is expected to be more exact. */

    const auto delta_t =
      basic.pressure_altitude_available.GetTimeDifference(last.pressure_altitude_available);

    if (delta_t > 0) {
      /* only update when a new value was received */

      auto delta_h = basic.pressure_altitude - last.pressure_altitude;
      auto delta_e = basic.energy_height - last.energy_height;

      basic.gps_vario = delta_h / delta_t;
      basic.gps_vario_TE = (delta_h + delta_e) / delta_t;
      basic.gps_vario_available = basic.pressure_altitude_available;
    }
  } else if (basic.baro_altitude_available && last.baro_altitude_available) {
    /* barometric altitude is also ok, but it's rare that it is
       available when pressure altitude is not */

    const auto delta_t =
      basic.baro_altitude_available.GetTimeDifference(last.baro_altitude_available);

    if (delta_t > 0) {
      /* only update when a new value was received */

      auto delta_h = basic.baro_altitude - last.baro_altitude;
      auto delta_e = basic.energy_height - last.energy_height;

      basic.gps_vario = delta_h / delta_t;
      basic.gps_vario_TE = (delta_h + delta_e) / delta_t;
      basic.gps_vario_available = basic.baro_altitude_available;
    }
  } else if (basic.gps_altitude_available && last_gps.gps_altitude_available &&
             basic.time_available && last_gps.time_available) {
    /* use the GPS time stamp, because it reflects when this altitude
       was measured by the GPS receiver; the Validity object just
       shows when this value was parsed by XCSoar */
    const auto delta_t = basic.time - last_gps.time;

    if (delta_t > 0) {
      /* only update when a new value was received */

      auto delta_h = basic.gps_altitude - last_gps.gps_altitude;
      auto delta_e = basic.energy_height - last_gps.energy_height;

      basic.gps_vario = delta_h / delta_t;
      basic.gps_vario_TE = (delta_h + delta_e) / delta_t;
      basic.gps_vario_available = basic.gps_altitude_available;
    }
  } else {
    basic.gps_vario = basic.gps_vario_TE = 0;
    basic.gps_vario_available.Clear();
  }
}

static void
ComputeBruttoVario(MoreData &basic)
{
  if (basic.total_energy_vario_available) {
    basic.brutto_vario = basic.total_energy_vario;
    basic.brutto_vario_available = basic.total_energy_vario_available;
  } else {
    basic.brutto_vario = basic.gps_vario;
    basic.brutto_vario_available = basic.gps_vario_available;
  }
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
 * Calculates the estimated bank and pitch angles
 */
static void
ComputeDynamics(MoreData &basic, const DerivedInfo &calculated)
{
  if (!calculated.flight.flying)
    return;

  if (basic.ground_speed <= 0 &&
      (!calculated.wind_available || calculated.wind.IsZero()))
    return;

  if (!basic.airspeed_available)
    return;

  // estimate bank angle (assuming balanced turn)
  const auto angle = atan((calculated.turn_rate_heading
      * basic.true_airspeed * INVERSE_G).Radians());

  if (!basic.attitude.bank_angle_available) {
    basic.attitude.bank_angle = Angle::Radians(angle);
    basic.attitude.bank_angle_computed = true;
  }

  if (!basic.attitude.pitch_angle_available && basic.total_energy_vario_available) {
    // estimate pitch angle (assuming balanced turn)
    basic.attitude.pitch_angle = Angle::FromXY(basic.true_airspeed,
                                               basic.gps_vario - basic.total_energy_vario);
    basic.attitude.pitch_angle_computed = true;
  }

  if (!basic.acceleration.available)
    basic.acceleration.ProvideGLoad(1. / std::max(0.001, fabs(cos(angle))), false);
}

void
BasicComputer::Fill(MoreData &data, const AtmosphericPressure qnh,
                    const FeaturesSettings &features)
{
  FillVario(data);
  ComputePressure(data, qnh);
  ComputeNavAltitude(data, features);
}

void
BasicComputer::Fill(MoreData &data, const ComputerSettings &settings_computer)
{
  const AtmosphericPressure qnh = settings_computer.pressure_available
    ? settings_computer.pressure
    : AtmosphericPressure::Zero();
  Fill(data, qnh, settings_computer.features);
}

void
BasicComputer::Compute(MoreData &data,
                       const MoreData &last, const MoreData &last_gps,
                       const DerivedInfo &calculated)
{
  ComputeTrack(data, last_gps);

  ground_speed.Compute(data);

  ComputeAirspeed(data, calculated);

  ComputeHeading(data.attitude, data, calculated);

  ComputeEnergyHeight(data);
  ComputeGPSVario(data, last, last_gps);
  ComputeBruttoVario(data);
  ComputeNettoVario(data, calculated);
  ComputeDynamics(data, calculated);
}
