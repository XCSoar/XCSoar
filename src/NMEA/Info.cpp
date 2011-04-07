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
SWITCH_INFO::reset()
{
  AirbrakeLocked = false;
  FlapPositive = false;
  FlapNeutral = false;
  FlapNegative = false;
  GearExtended = false;
  Acknowledge = false;
  Repeat = false;
  SpeedCommand = false;
  UserSwitchUp = false;
  UserSwitchMiddle = false;
  UserSwitchDown = false;
  FlightMode = SWITCH_INFO::MODE_UNKNOWN;
}

void
GPS_STATE::reset()
{
  real = false;
  Simulator = false;
#ifdef ANDROID
  AndroidInternalGPS = false;
#endif
  SatellitesUsed = 0;
  MovementDetected = false;
  Replay = false;
}

void
ACCELERATION_STATE::complement(const ACCELERATION_STATE &add)
{
  /* calculated: BankAngle, PitchAngle */

  if (!Available && add.Available) {
    Gload = add.Gload;
    Available = add.Available;
  }
}

void
NMEA_INFO::reset()
{
  Connected.clear();

  gps.reset();
  acceleration.reset();

  LocationAvailable.clear();

  TrackBearing = Angle::native(fixed_zero);
  TrackBearingAvailable.clear();

  GroundSpeedAvailable.clear();
  AirspeedAvailable.clear();
  GroundSpeed = TrueAirspeed = IndicatedAirspeed = fixed_zero;

  GPSAltitudeAvailable.clear();
  GPSAltitude = fixed_zero;

  static_pressure_available.clear();

  BaroAltitudeAvailable.clear();
  BaroAltitude = fixed_zero;

  PressureAltitudeAvailable.clear();
  PressureAltitude = fixed_zero;

  NavAltitude = fixed_zero;

  pressure.set_QNH(fixed(1013.25));
  QNHAvailable.clear();

  Time = fixed_zero;
  DateTime.year = DateTime.month = DateTime.day = 0;
  DateTime.day_of_week = 0;
  DateTime.hour = DateTime.minute = DateTime.second = 0;

  TotalEnergyVarioAvailable.clear();
  NettoVarioAvailable.clear();

  settings.Clear();

  ExternalWindAvailable.clear();

  TemperatureAvailable = false;
  HumidityAvailable = false;

  engine_noise_level_available.clear();

  SupplyBatteryVoltageAvailable.clear();

  SwitchStateAvailable = false;
  SwitchState.reset();

  // XXX StallRatio

  flarm.clear();
}

void
NMEA_INFO::ResetFlight(bool full)
{
  if (full) {
    Time = fixed_zero;
    BaroAltitudeAvailable.clear();
  }
}

void
NMEA_INFO::expire_wall_clock()
{
  const fixed monotonic = fixed(MonotonicClockMS()) / 1000;
  Connected.expire(monotonic, fixed(10));
  if (!Connected) {
    gps.reset();
    flarm.clear();
  }
}

void
NMEA_INFO::expire()
{
  LocationAvailable.expire(Time, fixed(10));
  TrackBearingAvailable.expire(Time, fixed(10));
  GroundSpeedAvailable.expire(Time, fixed(10));
  AirspeedAvailable.expire(Time, fixed(30));
  GPSAltitudeAvailable.expire(Time, fixed(30));
  static_pressure_available.expire(Time, fixed(30));
  BaroAltitudeAvailable.expire(Time, fixed(30));
  PressureAltitudeAvailable.expire(Time, fixed(30));
  TotalEnergyVarioAvailable.expire(Time, fixed(5));
  NettoVarioAvailable.expire(Time, fixed(5));
  settings.Expire(Time);
  ExternalWindAvailable.expire(Time, fixed(600));
  engine_noise_level_available.expire(Time, fixed(30));
    SupplyBatteryVoltageAvailable.expire(Time, fixed(300));
  flarm.Refresh(Time);
}

void
NMEA_INFO::complement(const NMEA_INFO &add)
{
  if (!add.Connected)
    /* if there is no heartbeat on the other object, there cannot be
       useful information */
    return;

  if (!Connected) {
    gps = add.gps;
    Time = add.Time;
    DateTime = add.DateTime;
  }

  Connected.complement(add.Connected);

  acceleration.complement(add.acceleration);

  if (LocationAvailable.complement(add.LocationAvailable))
    Location = add.Location;

  if (TrackBearingAvailable.complement(add.TrackBearingAvailable))
    TrackBearing = add.TrackBearing;

  if (GroundSpeedAvailable.complement(add.GroundSpeedAvailable))
    GroundSpeed = add.GroundSpeed;

  if (AirspeedAvailable.complement(add.AirspeedAvailable)) {
    TrueAirspeed = add.TrueAirspeed;
    IndicatedAirspeed = add.IndicatedAirspeed;
  }

  if (GPSAltitudeAvailable.complement(add.GPSAltitudeAvailable))
    GPSAltitude = add.GPSAltitude;

  if (static_pressure_available.complement(add.static_pressure_available))
    static_pressure = add.static_pressure;

  if (BaroAltitudeAvailable.complement(add.BaroAltitudeAvailable))
    BaroAltitude = add.BaroAltitude;

  if (PressureAltitudeAvailable.complement(add.PressureAltitudeAvailable))
    PressureAltitude = add.PressureAltitude;

  /* calculated: working_band_height,
     NavAltitude,working_band_fraction */

  /* managed by DeviceBlackboard: pressure */

  if (TotalEnergyVarioAvailable.complement(add.TotalEnergyVarioAvailable))
    TotalEnergyVario = add.TotalEnergyVario;

  if (NettoVarioAvailable.complement(add.NettoVarioAvailable))
    NettoVario = add.NettoVario;

  settings.Complement(add.settings);

  if (ExternalWindAvailable.complement(add.ExternalWindAvailable))
    ExternalWind = add.ExternalWind;

  if (!TemperatureAvailable && add.TemperatureAvailable) {
    OutsideAirTemperature = add.OutsideAirTemperature;
    TemperatureAvailable = add.TemperatureAvailable;
  }

  if (!HumidityAvailable && add.HumidityAvailable) {
    RelativeHumidity = add.RelativeHumidity;
    HumidityAvailable = add.HumidityAvailable;
  }

  if (SupplyBatteryVoltageAvailable.complement(add.SupplyBatteryVoltageAvailable))
    SupplyBatteryVoltage = add.SupplyBatteryVoltage;

  if (!SwitchStateAvailable && add.SwitchStateAvailable)
    SwitchState = add.SwitchState;

  flarm.complement(add.flarm);
}
