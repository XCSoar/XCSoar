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
  satellites_used = 0;
  replay = false;
}

void
NMEA_INFO::UpdateClock()
{
  clock = fixed(MonotonicClockMS()) / 1000;
}

void
NMEA_INFO::Reset()
{
  UpdateClock();

  Connected.Clear();

  gps.Reset();
  acceleration.Reset();

  LocationAvailable.Clear();

  track = Angle::native(fixed_zero);
  track_available.Clear();

  GroundSpeedAvailable.Clear();
  AirspeedAvailable.Clear();
  GroundSpeed = TrueAirspeed = IndicatedAirspeed = fixed_zero;
  AirspeedReal = false;

  GPSAltitudeAvailable.Clear();

  static_pressure_available.Clear();

  BaroAltitudeAvailable.Clear();
  BaroAltitude = fixed_zero;

  PressureAltitudeAvailable.Clear();
  PressureAltitude = fixed_zero;

  DateAvailable = false;

  time_available.Clear();
  Time = fixed_zero;
  DateTime.hour = DateTime.minute = DateTime.second = 0;

  TotalEnergyVarioAvailable.Clear();
  NettoVarioAvailable.Clear();

  settings.Clear();

  ExternalWindAvailable.Clear();

  TemperatureAvailable = false;
  HumidityAvailable = false;

  engine_noise_level_available.Clear();

  SupplyBatteryVoltageAvailable.Clear();

  SwitchStateAvailable = false;
  switch_state.Reset();

  StallRatioAvailable.Clear();

  // XXX StallRatio

  flarm.clear();
}

void
NMEA_INFO::ExpireWallClock()
{
  if (!Connected)
    return;

  UpdateClock();

#ifdef ANDROID
  if (gps.android_internal_gps)
    /* the Android internal GPS does not expire */
    return;
#endif

  Connected.Expire(clock, fixed(10));
  if (!Connected) {
    time_available.Clear();
    gps.Reset();
    flarm.clear();
  } else {
    time_available.Expire(clock, fixed(10));
  }
}

void
NMEA_INFO::Expire()
{
  LocationAvailable.Expire(clock, fixed(10));
  track_available.Expire(clock, fixed(10));
  GroundSpeedAvailable.Expire(clock, fixed(10));

  if (AirspeedAvailable.Expire(clock, fixed(30)))
    AirspeedReal = false;

  GPSAltitudeAvailable.Expire(clock, fixed(30));
  static_pressure_available.Expire(clock, fixed(30));
  BaroAltitudeAvailable.Expire(clock, fixed(30));
  PressureAltitudeAvailable.Expire(clock, fixed(30));
  TotalEnergyVarioAvailable.Expire(clock, fixed(5));
  NettoVarioAvailable.Expire(clock, fixed(5));
  settings.Expire(clock);
  ExternalWindAvailable.Expire(clock, fixed(600));
  engine_noise_level_available.Expire(clock, fixed(30));
  SupplyBatteryVoltageAvailable.Expire(clock, fixed(300));
  flarm.Refresh(clock);
}

void
NMEA_INFO::Complement(const NMEA_INFO &add)
{
  if (!add.Connected)
    /* if there is no heartbeat on the other object, there cannot be
       useful information */
    return;

  if (!Connected) {
    gps = add.gps;
  }

  Connected.Complement(add.Connected);

  if (time_available.Complement(add.time_available)) {
    Time = add.Time;
    DateTime = add.DateTime;
  }

  acceleration.Complement(add.acceleration);

  if (LocationAvailable.Complement(add.LocationAvailable))
    Location = add.Location;

  if (track_available.Complement(add.track_available))
    track = add.track;

  if (GroundSpeedAvailable.Complement(add.GroundSpeedAvailable))
    GroundSpeed = add.GroundSpeed;

  if ((add.AirspeedReal || !AirspeedReal) &&
      AirspeedAvailable.Complement(add.AirspeedAvailable)) {
    TrueAirspeed = add.TrueAirspeed;
    IndicatedAirspeed = add.IndicatedAirspeed;
    AirspeedReal = add.AirspeedReal;
  }

  if (GPSAltitudeAvailable.Complement(add.GPSAltitudeAvailable))
    GPSAltitude = add.GPSAltitude;

  if (static_pressure_available.Complement(add.static_pressure_available))
    static_pressure = add.static_pressure;

  if (BaroAltitudeAvailable.Complement(add.BaroAltitudeAvailable))
    BaroAltitude = add.BaroAltitude;

  if (PressureAltitudeAvailable.Complement(add.PressureAltitudeAvailable))
    PressureAltitude = add.PressureAltitude;

  if (TotalEnergyVarioAvailable.Complement(add.TotalEnergyVarioAvailable)) {
    TotalEnergyVario = add.TotalEnergyVario;
  }

  if (NettoVarioAvailable.Complement(add.NettoVarioAvailable))
    NettoVario = add.NettoVario;

  settings.Complement(add.settings);

  if (ExternalWindAvailable.Complement(add.ExternalWindAvailable))
    ExternalWind = add.ExternalWind;

  if (!TemperatureAvailable && add.TemperatureAvailable) {
    OutsideAirTemperature = add.OutsideAirTemperature;
    TemperatureAvailable = add.TemperatureAvailable;
  }

  if (!HumidityAvailable && add.HumidityAvailable) {
    RelativeHumidity = add.RelativeHumidity;
    HumidityAvailable = add.HumidityAvailable;
  }

  if (SupplyBatteryVoltageAvailable.Complement(add.SupplyBatteryVoltageAvailable))
    SupplyBatteryVoltage = add.SupplyBatteryVoltage;

  if (!SwitchStateAvailable && add.SwitchStateAvailable)
    switch_state = add.switch_state;

  if (!StallRatioAvailable && add.StallRatioAvailable)
    StallRatio = add.StallRatio;

  flarm.complement(add.flarm);
}
