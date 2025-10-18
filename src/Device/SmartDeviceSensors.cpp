// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include "SmartDeviceSensors.hpp"

#include "Descriptor.hpp"
#include "DataEditor.hpp"
#include "NMEA/Info.hpp"
#include "Geo/Geoid.hpp"


void
DeviceDescriptor::OnConnected(int connected) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();

  switch (connected) {
  case 0: /* not connected */
    basic.alive.Clear();
    basic.location_available.Clear();
    break;

  case 1: /* waiting for fix */
    basic.alive.Update(basic.clock);
    basic.gps.nonexpiring_internal_gps = true;
    basic.location_available.Clear();
    break;

  case 2: /* connected */
    basic.alive.Update(basic.clock);
    basic.gps.nonexpiring_internal_gps = true;
    break;
  }

  e.Commit();
}

void
DeviceDescriptor::OnLocationSensor(std::chrono::system_clock::time_point time,
                                   int n_satellites,
                                   GeoPoint location,
                                   bool hasAltitude, bool geoid_altitude,
                                   double altitude,
                                   bool hasBearing, double bearing,
                                   bool hasSpeed, double ground_speed,
                                   bool hasAccuracy, double accuracy) noexcept
{
  const auto e = BeginEdit();
  NMEAInfo &basic = *e;
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  const BrokenDateTime date_time{time};
  const BrokenDateTime midnight = date_time.AtMidnight();
  TimeStamp second_of_day{
    duration_cast<FloatDuration>(time - midnight.ToTimePoint())};

  basic.time = second_of_day;
  basic.time_available.Update(basic.clock);
  basic.date_time_utc = date_time;

  basic.gps.satellites_used = n_satellites;
  basic.gps.satellites_used_available.Update(basic.clock);
  basic.gps.real = true;
  basic.gps.nonexpiring_internal_gps = true;
  basic.location = location;
  basic.location_available.Update(basic.clock);

  if (hasAltitude) {
    auto GeoidSeparation = geoid_altitude
      ? 0.
      : EGM96::LookupSeparation(basic.location);
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

  e.Commit();
}


#if defined(ANDROID) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)

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

  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                kalman_filter.GetXVel()));
  basic.ProvideStaticPressure(
      AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));

  e.Commit();
}
#endif
