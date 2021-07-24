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

#include "Descriptor.hpp"
#include "DataEditor.hpp"
#include "NMEA/Info.hpp"
#include "Geo/Geoid.hpp"
#include "system/Clock.hpp"

void
DeviceDescriptor::OnConnected(int connected) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;

  switch (connected) {
  case 0: /* not connected */
    basic.alive.Clear();
    basic.location_available.Clear();
    break;

  case 1: /* waiting for fix */
    basic.alive.Update(MonotonicClockFloat());
    basic.gps.nonexpiring_internal_gps = true;
    basic.location_available.Clear();
    break;

  case 2: /* connected */
    basic.alive.Update(MonotonicClockFloat());
    basic.gps.nonexpiring_internal_gps = true;
    break;
  }

  e.Commit();
}

void
DeviceDescriptor::OnLocationSensor(long time, int n_satellites,
                                   double longitude, double latitude,
                                   bool hasAltitude, double altitude,
                                   bool hasBearing, double bearing,
                                   bool hasSpeed, double ground_speed,
                                   bool hasAccuracy, double accuracy,
                                   bool hasAcceleration,
                                   double acceleration) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  BrokenDateTime date_time = BrokenDateTime::FromUnixTimeUTC(time / 1000);
  double second_of_day = date_time.GetSecondOfDay() +
    /* add the millisecond fraction of the original timestamp for
       better accuracy */
    unsigned(time % 1000) / 1000.;

  if (second_of_day < basic.time &&
      basic.date_time_utc.IsDatePlausible() &&
      (BrokenDate)date_time > (BrokenDate)basic.date_time_utc)
    /* don't wrap around when going past midnight in UTC */
    second_of_day += 24u * 3600u;

  basic.time = second_of_day;
  basic.time_available.Update(basic.clock);
  basic.date_time_utc = date_time;

  basic.gps.satellites_used = n_satellites;
  basic.gps.satellites_used_available.Update(basic.clock);
  basic.gps.real = true;
  basic.gps.nonexpiring_internal_gps = true;
  basic.location = GeoPoint(Angle::Degrees(longitude),
                            Angle::Degrees(latitude));
  basic.location_available.Update(basic.clock);

  if (hasAltitude) {
    auto GeoidSeparation = EGM96::LookupSeparation(basic.location);
    basic.gps_altitude = altitude - GeoidSeparation;
    basic.gps_altitude_available.Update(basic.clock);
  } else
    basic.gps_altitude_available.Clear();

  if (hasBearing) {
    basic.track = Angle::Degrees(bearing);
    basic.track_available.Update(basic.clock);
  } else
    basic.track_available.Clear();

  if (hasSpeed) {
    basic.ground_speed = ground_speed;
    basic.ground_speed_available.Update(basic.clock);
  }

  basic.gps.hdop = hasAccuracy ? accuracy : -1;

  if (hasAcceleration)
    basic.acceleration.ProvideGLoad(acceleration, true);

  e.Commit();
}


void
DeviceDescriptor::OnAccelerationSensor(float ddx, float ddy,
                                       float ddz) noexcept
{
  // TODO
}

void
DeviceDescriptor::OnRotationSensor(float dtheta_x, float dtheta_y,
                                   float dtheta_z) noexcept
{
  // TODO
}

void
DeviceDescriptor::OnMagneticFieldSensor(float h_x, float h_y,
                                        float h_z) noexcept
{
  // TODO
}

/**
 * Helper function for
 * Java_org_xcsoar_NonGPSSensors_setBarometricPressure: Given a
 * current measurement of the atmospheric pressure and the rate of
 * change of atmospheric pressure (in millibars and millibars per
 * second), compute the uncompensated vertical speed of the glider in
 * meters per second, assuming standard atmospheric conditions
 * (deviations from these conditions should not matter very
 * much). This calculation can be derived by taking the formula for
 * converting barometric pressure to pressure altitude (see e.g.
 * http://psas.pdx.edu/RocketScience/PressureAltitude_Derived.pdf),
 * expressing it as a function P(t), the atmospheric pressure at time
 * t, then taking the derivative with respect to t. The dP(t)/dt term
 * is the pressure change rate.
 */
gcc_pure
static inline double
ComputeNoncompVario(const double pressure, const double d_pressure)
{
  static constexpr double FACTOR(-2260.389548275485);
  static constexpr double EXPONENT(-0.8097374740609689);
  return FACTOR * pow(pressure, EXPONENT) * d_pressure;
}

void
DeviceDescriptor::OnBarometricPressureSensor(float pressure,
                                             float sensor_noise_variance) noexcept
{
  /* Kalman filter updates are also protected by the blackboard
     mutex. These should not take long; we won't hog the mutex
     unduly. */
  kalman_filter.Update(pressure, sensor_noise_variance);

  const auto e = BeginEdit();
  NMEAInfo &basic = *e;

  basic.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                kalman_filter.GetXVel()));
  basic.ProvideStaticPressure(
      AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));

  e.Commit();
}
