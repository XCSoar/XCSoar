// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SmartDeviceSensors.hpp"

#include "Logger/NMEALogger.hpp"
#include "NMEA/Checksum.hpp"
#include "Descriptor.hpp"
#include "DataEditor.hpp"
#include "NMEA/Info.hpp"
#include "Geo/Geoid.hpp"

static constexpr int MIN_SATELLITES_FOR_3D_FIX = 4; // 3D fix needs ≥4 satellites

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

  const double geoid_separation = EGM96::LookupSeparation(basic.location);
  const double altitude_correction = geoid_altitude ? 0. : geoid_separation;
  if (hasAltitude) {
    basic.gps_altitude = altitude - altitude_correction;
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

  // synthesize GPRMC and GPGGA sentences and write to NMEA Logger
  if (nmea_logger != nullptr &&
      nmea_logger->IsEnabled() &&
      n_satellites >= MIN_SATELLITES_FOR_3D_FIX) {
    char sentence[128];
    const int lat_deg = std::abs((int)location.latitude.Degrees());
    const int lon_deg = std::abs((int)location.longitude.Degrees());
    const double lat_frac = std::abs(location.latitude.Degrees()) - lat_deg;
    const double lon_frac = std::abs(location.longitude.Degrees()) - lon_deg;
    const char lat_hem = (location.latitude.Degrees() < 0.0) ? 'S' : 'N';
    const char lon_hem = (location.longitude.Degrees() < 0.0) ? 'W' : 'E';

    char loc_str[32];
    snprintf(loc_str, sizeof(loc_str), "%02d%07.4f,%c,%03d%07.4f,%c",
              lat_deg, lat_frac*60.0, lat_hem,
              lon_deg, lon_frac*60.0, lon_hem);
    char ts_str[16];
    snprintf(ts_str, sizeof(ts_str), "%02d%02d%02d.00",
              date_time.hour,date_time.minute,date_time.second);
    char speed_str[12] = "";
    if (hasSpeed)
      snprintf(speed_str, sizeof(speed_str), "%.2f",
               ground_speed * 1.94384); // convert m/s to kn
    char cog_str[12] = "";
    if (hasBearing)
      snprintf(cog_str, sizeof(cog_str), "%.2f", bearing);
    char alt_geoid_str[32] = ",M,,M";
    if (hasAltitude)
      snprintf(alt_geoid_str, sizeof(alt_geoid_str),
               "%.1f,M,%.1f,M",
               basic.gps_altitude, geoid_separation);
    char hdop_str[12] = "";
    if (hasAccuracy)
      snprintf(hdop_str, sizeof(hdop_str), "%.2f", accuracy);

    int length = snprintf(sentence, sizeof(sentence),
             "$GPRMC,%s,A,%s,%s,%s,%02d%02d%02d,,,A",
             ts_str, loc_str, speed_str, cog_str,
             date_time.day, date_time.month, date_time.year % 100);
    if (length >= 0 &&
        length <= static_cast<int>(sizeof(sentence)) - 4) {
      AppendNMEAChecksum(sentence);
      nmea_logger->Log(sentence);
    }

    length = snprintf(sentence, sizeof(sentence),
             "$GPGGA,%s,%s,1,%d,%s,%s,,",
             ts_str, loc_str, n_satellites, hdop_str, alt_geoid_str);
    if (length >= 0 &&
        length <= static_cast<int>(sizeof(sentence)) - 4) {
      AppendNMEAChecksum(sentence);
      nmea_logger->Log(sentence);
    }
  }
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

  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                kalman_filter.GetXVel()));
  basic.ProvideStaticPressure(
      AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));

  e.Commit();
}
