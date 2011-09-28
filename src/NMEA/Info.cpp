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

#include "NMEA/Info.hpp"
#include "OS/Clock.hpp"

void
SwitchInfo::Reset()
{
  airbrake_locked = false;
  flap_positive = false;
  flap_neutral = false;
  flap_negative = false;
  gear_extended = false;
  acknowledge = false;
  repeat = false;
  speed_command = false;
  user_switch_up = false;
  user_switch_middle = false;
  user_switch_down = false;
  flight_mode = SwitchInfo::MODE_UNKNOWN;
}

void
GPSState::Reset()
{
  real = false;
  simulator = false;
#ifdef ANDROID
  android_internal_gps = false;
#endif
  satellites_used = -1;
  replay = false;
}

void
NMEAInfo::UpdateClock()
{
  clock = fixed(MonotonicClockMS()) / 1000;
}

void
NMEAInfo::ProvideTime(fixed _time)
{
  assert(!negative(_time));

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
NMEAInfo::Reset()
{
  UpdateClock();

  connected.Clear();

  gps.Reset();
  acceleration.Reset();

  location_available.Clear();

  track = Angle::zero();
  track_available.Clear();

  ground_speed_available.Clear();
  airspeed_available.Clear();
  ground_speed = true_airspeed = indicated_airspeed = fixed_zero;
  airspeed_real = false;

  gps_altitude_available.Clear();

  static_pressure_available.Clear();

  baro_altitude_available.Clear();
  baro_altitude = fixed_zero;

  pressure_altitude_available.Clear();
  pressure_altitude = fixed_zero;

  date_available = false;

  time_available.Clear();
  time = fixed_zero;
  date_time_utc.hour = date_time_utc.minute = date_time_utc.second = 0;

  total_energy_vario_available.Clear();
  netto_vario_available.Clear();

  settings.Clear();

  external_wind_available.Clear();

  temperature_available = false;
  humidity_available = false;

  engine_noise_level_available.Clear();

  voltage_available.Clear();

  switch_state_available = false;
  switch_state.Reset();

  stall_ratio_available.Clear();

  // XXX StallRatio

  flarm.Clear();
}

void
NMEAInfo::ExpireWallClock()
{
  if (!connected)
    return;

  UpdateClock();

#ifdef ANDROID
  if (gps.android_internal_gps)
    /* the Android internal GPS does not expire */
    return;
#endif

  connected.Expire(clock, fixed(10));
  if (!connected) {
    time_available.Clear();
    gps.Reset();
    flarm.Clear();
  } else {
    time_available.Expire(clock, fixed(10));
  }
}

void
NMEAInfo::Expire()
{
  location_available.Expire(clock, fixed(10));
  track_available.Expire(clock, fixed(10));
  ground_speed_available.Expire(clock, fixed(10));

  if (airspeed_available.Expire(clock, fixed(30)))
    airspeed_real = false;

  gps_altitude_available.Expire(clock, fixed(30));
  static_pressure_available.Expire(clock, fixed(30));
  baro_altitude_available.Expire(clock, fixed(30));
  pressure_altitude_available.Expire(clock, fixed(30));
  total_energy_vario_available.Expire(clock, fixed(5));
  netto_vario_available.Expire(clock, fixed(5));
  settings.Expire(clock);
  external_wind_available.Expire(clock, fixed(600));
  engine_noise_level_available.Expire(clock, fixed(30));
  voltage_available.Expire(clock, fixed(300));
  flarm.Refresh(clock);
}

void
NMEAInfo::Complement(const NMEAInfo &add)
{
  if (!add.connected)
    /* if there is no heartbeat on the other object, there cannot be
       useful information */
    return;

  if (!connected) {
    gps = add.gps;
  }

  connected.Complement(add.connected);

  if (time_available.Complement(add.time_available)) {
    time = add.time;
    date_time_utc = add.date_time_utc;
  }

  acceleration.Complement(add.acceleration);

  if (location_available.Complement(add.location_available))
    location = add.location;

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

  if (baro_altitude_available.Complement(add.baro_altitude_available))
    baro_altitude = add.baro_altitude;

  if (pressure_altitude_available.Complement(add.pressure_altitude_available))
    pressure_altitude = add.pressure_altitude;

  if (total_energy_vario_available.Complement(add.total_energy_vario_available)) {
    total_energy_vario = add.total_energy_vario;
  }

  if (netto_vario_available.Complement(add.netto_vario_available))
    netto_vario = add.netto_vario;

  settings.Complement(add.settings);

  if (external_wind_available.Complement(add.external_wind_available))
    external_wind = add.external_wind;

  if (!temperature_available && add.temperature_available) {
    temperature = add.temperature;
    temperature_available = add.temperature_available;
  }

  if (!humidity_available && add.humidity_available) {
    humidity = add.humidity;
    humidity_available = add.humidity_available;
  }

  if (voltage_available.Complement(add.voltage_available))
    voltage = add.voltage;

  if (!switch_state_available && add.switch_state_available)
    switch_state = add.switch_state;

  if (!stall_ratio_available && add.stall_ratio_available)
    stall_ratio = add.stall_ratio;

  flarm.Complement(add.flarm);
}
