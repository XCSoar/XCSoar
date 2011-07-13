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
  Replay = false;
}

void
NMEA_INFO::reset()
{
  Connected.Clear();

  gps.reset();
  acceleration.reset();

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

  NavAltitude = fixed_zero;

  Time = fixed_zero;
  DateTime.hour = DateTime.minute = DateTime.second = 0;

  TotalEnergyVarioAvailable.Clear();
  NettoVarioAvailable.Clear();

  VarioCounter = 0;

  settings.Clear();

  ExternalWindAvailable.Clear();

  TemperatureAvailable = false;
  HumidityAvailable = false;

  engine_noise_level_available.Clear();

  SupplyBatteryVoltageAvailable.Clear();

  SwitchStateAvailable = false;
  SwitchState.reset();

  StallRatioAvailable.Clear();

  // XXX StallRatio

  flarm.clear();
}

void
NMEA_INFO::expire_wall_clock()
{
#ifdef ANDROID
  if (gps.AndroidInternalGPS)
    /* the Android internal GPS does not expire */
    return;
#endif

  const fixed monotonic = fixed(MonotonicClockMS()) / 1000;
  Connected.Expire(monotonic, fixed(10));
  if (!Connected) {
    gps.reset();
    flarm.clear();
  }
}

void
NMEA_INFO::expire()
{
  LocationAvailable.Expire(Time, fixed(10));
  track_available.Expire(Time, fixed(10));
  GroundSpeedAvailable.Expire(Time, fixed(10));

  if (AirspeedAvailable.Expire(Time, fixed(30)))
    AirspeedReal = false;

  GPSAltitudeAvailable.Expire(Time, fixed(30));
  static_pressure_available.Expire(Time, fixed(30));
  BaroAltitudeAvailable.Expire(Time, fixed(30));
  PressureAltitudeAvailable.Expire(Time, fixed(30));
  TotalEnergyVarioAvailable.Expire(Time, fixed(5));
  NettoVarioAvailable.Expire(Time, fixed(5));
  settings.Expire(Time);
  ExternalWindAvailable.Expire(Time, fixed(600));
  engine_noise_level_available.Expire(Time, fixed(30));
    SupplyBatteryVoltageAvailable.Expire(Time, fixed(300));
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

  Connected.Complement(add.Connected);

  acceleration.complement(add.acceleration);

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
    VarioCounter = add.VarioCounter;
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
    SwitchState = add.SwitchState;

  if (!StallRatioAvailable && add.StallRatioAvailable)
    StallRatio = add.StallRatio;

  flarm.complement(add.flarm);
}
