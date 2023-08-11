// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Info.hpp"
#include "Atmosphere/AirDensity.hpp"
#include "time/Cast.hxx"

void
NMEAInfo::UpdateClock() noexcept
{
  clock = TimeStamp{std::chrono::steady_clock::now().time_since_epoch()};
}

BrokenDateTime
NMEAInfo::GetDateTimeAt(TimeStamp other_time) const noexcept
{
  if (!other_time.IsDefined())
    return BrokenDateTime::Invalid();

  if (!time_available || !date_time_utc.IsDatePlausible())
    return BrokenDateTime(BrokenDate::Invalid(),
                          BrokenTime::FromSinceMidnightChecked(other_time.ToDuration()));

  return date_time_utc + std::chrono::duration_cast<std::chrono::system_clock::duration>(FloatDuration{other_time - time});
}

void
NMEAInfo::ProvideTime(TimeStamp _time) noexcept
{
  assert(_time.IsDefined());

  time = _time;
  time_available.Update(clock);

  (BrokenTime &)date_time_utc = BrokenTime::FromSinceMidnightChecked(time.ToDuration());
}

void
NMEAInfo::ProvideDate(const BrokenDate &date) noexcept
{
  assert(date.IsPlausible());

  (BrokenDate &)date_time_utc = date;
}

void
NMEAInfo::ProvideTrueAirspeedWithAltitude(double tas, double altitude) noexcept
{
  true_airspeed = tas;
  indicated_airspeed = true_airspeed / AirDensityRatio(altitude);
  airspeed_available.Update(clock);
  airspeed_real = true;
}

void
NMEAInfo::ProvideIndicatedAirspeedWithAltitude(double ias,
                                               double altitude) noexcept
{
  indicated_airspeed = ias;
  true_airspeed = indicated_airspeed * AirDensityRatio(altitude);
  airspeed_available.Update(clock);
  airspeed_real = true;
}

void
NMEAInfo::ProvideTrueAirspeed(double tas) noexcept
{
  auto any_altitude = GetAnyAltitude();

  if (any_altitude)
    ProvideTrueAirspeedWithAltitude(tas, *any_altitude);
  else
    /* no altitude; dirty fallback */
    ProvideBothAirspeeds(tas, tas);
}

void
NMEAInfo::ProvideIndicatedAirspeed(double ias) noexcept
{
  auto any_altitude = GetAnyAltitude();

  if (any_altitude)
    ProvideIndicatedAirspeedWithAltitude(ias, *any_altitude);
  else
    /* no altitude; dirty fallback */
    ProvideBothAirspeeds(ias, ias);
}

void
NMEAInfo::Reset() noexcept
{
  UpdateClock();

  alive.Clear();

  gps.Reset();
  acceleration.Reset();
  attitude.Reset();

  location_available.Clear();

  track = Angle::Zero();
  track_available.Clear();

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
  time = {};

  date_time_utc = BrokenDateTime::Invalid();

  noncomp_vario_available.Clear();
  total_energy_vario_available.Clear();
  netto_vario_available.Clear();

  settings.Clear();

  external_wind_available.Clear();
  external_instantaneous_wind_available.Clear();

  temperature_available = false;
  humidity_available = false;

  heart_rate_available.Clear();

  engine_noise_level_available.Clear();

  voltage_available.Clear();
  battery_level_available.Clear();

  switch_state.Reset();

  stall_ratio_available.Clear();

  // XXX StallRatio

  device.Clear();
  secondary_device.Clear();
  flarm.Clear();

  engine.Reset();

#ifdef ANDROID
  glink_data.Clear();
#endif
}

void
NMEAInfo::ExpireWallClock() noexcept
{
  if (!alive)
    return;

  UpdateClock();

#if defined(ANDROID) || defined(__APPLE__)
  if (gps.nonexpiring_internal_gps)
    /* the internal GPS does not expire */
    return;
#endif

  alive.Expire(clock, std::chrono::seconds(10));
  if (!alive) {
    time_available.Clear();
    gps.Reset();
    flarm.Clear();

#ifdef ANDROID
    glink_data.Clear();
#endif
  } else {
    time_available.Expire(clock, std::chrono::seconds(10));
  }
}

void
NMEAInfo::Expire() noexcept
{
  if (location_available.Expire(clock, std::chrono::seconds(10)))
    /* if the location expires, then GPSState should expire as well,
       because all GPSState does is provide metadata for the GPS
       fix */
    gps.Reset();
  else
    gps.Expire(clock);

  track_available.Expire(clock, std::chrono::seconds(10));
  ground_speed_available.Expire(clock, std::chrono::seconds(10));

  if (airspeed_available.Expire(clock, std::chrono::seconds(30)))
    airspeed_real = false;

  gps_altitude_available.Expire(clock, std::chrono::seconds(30));
  static_pressure_available.Expire(clock, std::chrono::seconds(30));
  dyn_pressure_available.Expire(clock, std::chrono::seconds(30));
  pitot_pressure_available.Expire(clock, std::chrono::seconds(30));
  sensor_calibration_available.Expire(clock, std::chrono::hours(1));
  baro_altitude_available.Expire(clock, std::chrono::seconds(30));
  pressure_altitude_available.Expire(clock, std::chrono::seconds(30));
  noncomp_vario_available.Expire(clock, std::chrono::seconds(5));
  total_energy_vario_available.Expire(clock, std::chrono::seconds(5));
  netto_vario_available.Expire(clock, std::chrono::seconds(5));
  settings.Expire(clock);
  external_wind_available.Expire(clock, std::chrono::minutes(10));
  heart_rate_available.Expire(clock, std::chrono::seconds(10));
  engine_noise_level_available.Expire(clock, std::chrono::seconds(30));
  voltage_available.Expire(clock, std::chrono::minutes(5));
  battery_level_available.Expire(clock, std::chrono::minutes(5));
  flarm.Expire(clock);
  engine.Expire(clock);
#ifdef ANDROID
  glink_data.Expire(clock);
#endif
  attitude.Expire(clock);
}

void
NMEAInfo::Complement(const NMEAInfo &add) noexcept
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

  if (external_instantaneous_wind_available.Complement(
          add.external_instantaneous_wind_available))
    external_instantaneous_wind = add.external_instantaneous_wind;

  if (!temperature_available && add.temperature_available) {
    temperature = add.temperature;
    temperature_available = add.temperature_available;
  }

   if (!variation_available && add.variation_available) {
    variation = add.variation;
    variation_available = add.variation_available;
  }

  if (!humidity_available && add.humidity_available) {
    humidity = add.humidity;
    humidity_available = add.humidity_available;
  }

  if (heart_rate_available.Complement(add.heart_rate_available))
    heart_rate = add.heart_rate;

  if (engine_noise_level_available.Complement(add.engine_noise_level_available))
    engine_noise_level = add.engine_noise_level;

  if (voltage_available.Complement(add.voltage_available))
    voltage = add.voltage;

  if (battery_level_available.Complement(add.battery_level_available))
    battery_level = add.battery_level;

  switch_state.Complement(add.switch_state);

  if (!stall_ratio_available && add.stall_ratio_available)
    stall_ratio = add.stall_ratio;

  flarm.Complement(add.flarm);

  engine.Complement(add.engine);

#ifdef ANDROID
  glink_data.Complement(add.glink_data);
#endif
}
