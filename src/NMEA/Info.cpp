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

#include "NMEA/Info.hpp"
#include "OS/Clock.hpp"
#include "Atmosphere/AirDensity.hpp"

void
GPSState::Reset()
{
  fix_quality = FixQuality::NO_FIX;
  fix_quality_available.Clear();
  real = false;
  simulator = false;
#if defined(ANDROID) || defined(__APPLE__)
  nonexpiring_internal_gps = false;
#endif
  satellites_used_available.Clear();
  satellite_ids_available.Clear();
  replay = false;
}

void
GPSState::Expire(double now)
{
  if (fix_quality_available.Expire(now, 5))
    fix_quality = FixQuality::NO_FIX;

  satellites_used_available.Expire(now, 5);
  satellite_ids_available.Expire(now, 60);
}

void
NMEAInfo::UpdateClock()
{
  clock = MonotonicClockFloat();
}

BrokenDateTime
NMEAInfo::GetDateTimeAt(double other_time) const
{
  if (other_time < 0)
    return BrokenDateTime::Invalid();

  if (!time_available || !date_time_utc.IsDatePlausible())
    return BrokenDateTime(BrokenDate::Invalid(),
                          BrokenTime::FromSecondOfDayChecked(int(other_time)));

  return date_time_utc + int(other_time - time);
}

void
NMEAInfo::ProvideTime(double _time)
{
  assert(_time >= 0);

  time = _time;
  time_available.Update(clock);

  unsigned t = (unsigned)_time;
  date_time_utc.second = t % 60;
  t /= 60;

  date_time_utc.minute = t % 60;
  t /= 60;

  date_time_utc.hour = t % 24;
}

void
NMEAInfo::ProvideDate(const BrokenDate &date)
{
  assert(date.IsPlausible());

  (BrokenDate &)date_time_utc = date;
}

void
NMEAInfo::ProvideTrueAirspeedWithAltitude(double tas, double altitude)
{
  true_airspeed = tas;
  indicated_airspeed = true_airspeed / AirDensityRatio(altitude);
  airspeed_available.Update(clock);
  airspeed_real = true;
}

void
NMEAInfo::ProvideIndicatedAirspeedWithAltitude(double ias, double altitude)
{
  indicated_airspeed = ias;
  true_airspeed = indicated_airspeed * AirDensityRatio(altitude);
  airspeed_available.Update(clock);
  airspeed_real = true;
}

void
NMEAInfo::ProvideTrueAirspeed(double tas)
{
  auto any_altitude = GetAnyAltitude();

  if (any_altitude.first)
    ProvideTrueAirspeedWithAltitude(tas, any_altitude.second);
  else
    /* no altitude; dirty fallback */
    ProvideBothAirspeeds(tas, tas);
}

void
NMEAInfo::ProvideIndicatedAirspeed(double ias)
{
  auto any_altitude = GetAnyAltitude();

  if (any_altitude.first)
    ProvideIndicatedAirspeedWithAltitude(ias, any_altitude.second);
  else
    /* no altitude; dirty fallback */
    ProvideBothAirspeeds(ias, ias);
}

void
NMEAInfo::Reset()
{
  UpdateClock();

  alive.Clear();

  gps.Reset();
  acceleration.Reset();
  attitude.Reset();

  location_available.Clear();

  track = Angle::Zero();
  track_available.Clear();

  heading_available.Clear();

  variation_available.Clear();

  ground_speed_available.Clear();
  airspeed_available.Clear();
  ground_speed = true_airspeed = indicated_airspeed = 0;
  airspeed_real = false;

  gps_altitude_available.Clear();

  static_pressure_available.Clear();
  dyn_pressure_available.Clear();
  pitot_pressure_available.Clear();
  sensor_calibration_available.Clear();

  baro_altitude_available.Clear();
  baro_altitude = 0;

  pressure_altitude_available.Clear();
  pressure_altitude = 0;

  time_available.Clear();
  time = 0;

  date_time_utc = BrokenDateTime::Invalid();

  noncomp_vario_available.Clear();
  total_energy_vario_available.Clear();
  netto_vario_available.Clear();

  settings.Clear();

  external_wind_available.Clear();

  temperature_available = false;
  humidity_available = false;

  engine_noise_level_available.Clear();

  voltage_available.Clear();
  battery_level_available.Clear();

  switch_state.Reset();

  stall_ratio_available.Clear();

  // XXX StallRatio

  device.Clear();
  secondary_device.Clear();
  flarm.Clear();
}

void
NMEAInfo::ExpireWallClock()
{
  if (!alive)
    return;

  UpdateClock();

#if defined(ANDROID) || defined(__APPLE__)
  if (gps.nonexpiring_internal_gps)
    /* the internal GPS does not expire */
    return;
#endif

  alive.Expire(clock, 10);
  if (!alive) {
    time_available.Clear();
    gps.Reset();
    flarm.Clear();
  } else {
    time_available.Expire(clock, 10);
  }
}

void
NMEAInfo::Expire()
{
  location_available.Expire(clock, 10);
  track_available.Expire(clock, 10);
  ground_speed_available.Expire(clock, 10);

  if (airspeed_available.Expire(clock, 30))
    airspeed_real = false;

  gps_altitude_available.Expire(clock, 30);
  static_pressure_available.Expire(clock, 30);
  dyn_pressure_available.Expire(clock, 30);
  pitot_pressure_available.Expire(clock, 30);
  sensor_calibration_available.Expire(clock, 3600);
  baro_altitude_available.Expire(clock, 30);
  pressure_altitude_available.Expire(clock, 30);
  noncomp_vario_available.Expire(clock, 5);
  total_energy_vario_available.Expire(clock, 5);
  netto_vario_available.Expire(clock, 5);
  settings.Expire(clock);
  external_wind_available.Expire(clock, 600);
  engine_noise_level_available.Expire(clock, 30);
  voltage_available.Expire(clock, 300);
  battery_level_available.Expire(clock, 300);
  flarm.Expire(clock);
#ifdef ANDROID
  glink_data.Expire(clock);
#endif
  gps.Expire(clock);
  attitude.Expire(clock);
}

void
NMEAInfo::Complement(const NMEAInfo &add)
{
  if (!add.alive)
    /* if there is no heartbeat on the other object, there cannot be
       useful information */
    return;

  if (!alive) {
    gps = add.gps;
  }

  alive.Complement(add.alive);

  if (time_available.Complement(add.time_available)) {
    time = add.time;
    date_time_utc = add.date_time_utc;
  }

  acceleration.Complement(add.acceleration);
  attitude.Complement(add.attitude);

  if (location_available.Complement(add.location_available)) {
    location = add.location;

    /* the GPSState belongs to the device that provides the GPS fix */
    gps = add.gps;
  }

  if (track_available.Complement(add.track_available))
    track = add.track;

  if (ground_speed_available.Complement(add.ground_speed_available))
    ground_speed = add.ground_speed;

  if ((add.airspeed_real || !airspeed_real) &&
      airspeed_available.Complement(add.airspeed_available)) {
    true_airspeed = add.true_airspeed;
    indicated_airspeed = add.indicated_airspeed;
    airspeed_real = add.airspeed_real;
  }

  if (gps_altitude_available.Complement(add.gps_altitude_available))
    gps_altitude = add.gps_altitude;

  if (static_pressure_available.Complement(add.static_pressure_available))
    static_pressure = add.static_pressure;

  if (dyn_pressure_available.Complement(add.dyn_pressure_available))
    dyn_pressure = add.dyn_pressure;

  if (pitot_pressure_available.Complement(add.pitot_pressure_available))
    pitot_pressure = add.pitot_pressure;

  if (sensor_calibration_available.Complement(add.sensor_calibration_available)) {
     sensor_calibration_factor = add.sensor_calibration_factor;
     sensor_calibration_offset = add.sensor_calibration_offset;
  }

  if (baro_altitude_available.Complement(add.baro_altitude_available))
    baro_altitude = add.baro_altitude;

  if (pressure_altitude_available.Complement(add.pressure_altitude_available))
    pressure_altitude = add.pressure_altitude;

  if (noncomp_vario_available.Complement(add.noncomp_vario_available))
    noncomp_vario = add.noncomp_vario;

  if (total_energy_vario_available.Complement(add.total_energy_vario_available))
    total_energy_vario = add.total_energy_vario;

  if (netto_vario_available.Complement(add.netto_vario_available))
    netto_vario = add.netto_vario;

  settings.Complement(add.settings);

  if (external_wind_available.Complement(add.external_wind_available))
    external_wind = add.external_wind;

  if (!temperature_available && add.temperature_available) {
    temperature = add.temperature;
    temperature_available = add.temperature_available;
  }

  if (!heading_available && add.heading_available) {
    heading = add.heading;
    heading_available = add.heading_available;
  }

   if (!variation_available && add.variation_available) {
    variation = add.variation;
    variation_available = add.variation_available;
  }

  if (!humidity_available && add.humidity_available) {
    humidity = add.humidity;
    humidity_available = add.humidity_available;
  }

  if (voltage_available.Complement(add.voltage_available))
    voltage = add.voltage;

  if (battery_level_available.Complement(add.battery_level_available))
    battery_level = add.battery_level;

  switch_state.Complement(add.switch_state);

  if (!stall_ratio_available && add.stall_ratio_available)
    stall_ratio = add.stall_ratio;

  flarm.Complement(add.flarm);

#ifdef ANDROID
  glink_data.Complement(add.glink_data);
#endif
}
