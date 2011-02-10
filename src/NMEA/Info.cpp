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
  VarioCircling = false;
}

void
GPS_STATE::reset()
{
  // Set the NAVWarning positive (assume not gps found yet)
  Connected = 0;
  NAVWarning = true;
  Simulator = false;
  SatellitesUsed = 0;
  MovementDetected = false;
  Replay = false;
}

void
GPS_STATE::complement(const GPS_STATE &add) {
  if (add.Connected > Connected)
    *this = add;
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
  gps.reset();
  acceleration.reset();
  flight.flying_state_reset();

  // XXX Location

  TrackBearing = Heading = Angle::native(fixed_zero);
  TurnRateWind = TurnRate = fixed_zero;

  AirspeedAvailable.clear();
  GroundSpeed = TrueAirspeed = IndicatedAirspeed = fixed_zero;
  TrueAirspeedEstimated = fixed_zero;

  GPSAltitudeAvailable.clear();
  GPSAltitude = fixed_zero;

  BaroAltitudeAvailable.clear();
  BaroAltitudeOrigin = BARO_ALTITUDE_UNKNOWN;
  BaroAltitude = fixed_zero;
  EnergyHeight = TEAltitude = fixed_zero;

  working_band_height = working_band_ceiling = fixed_zero;

  NavAltitude = fixed_zero;

  working_band_fraction = fixed_zero;

  AltitudeAGL = fixed_zero;

  pressure.set_QNH(fixed(1013.25));

  Time = fixed_zero;
  DateTime.year = DateTime.month = DateTime.day = 0;
  DateTime.day_of_week = 0;
  DateTime.hour = DateTime.minute = DateTime.second = 0;

  GliderSinkRate = fixed_zero;

  GPSVario = GPSVarioTE = fixed_zero;

  TotalEnergyVarioAvailable.clear();
  NettoVarioAvailable.clear();
  BruttoVario = fixed_zero;

  // XXX MacCready, Ballast, Bugs

  ExternalWindAvailable.clear();
  WindAvailable.clear();

  TemperatureAvailable = false;
  HumidityAvailable = false;

  SupplyBatteryVoltage = fixed_minus_one;

  SwitchStateAvailable = false;
  SwitchState.reset();

  // XXX StallRatio

  flarm.clear();
}

void
NMEA_INFO::expire()
{
  AirspeedAvailable.expire(Time, fixed(30));
  GPSAltitudeAvailable.expire(Time, fixed(30));
  BaroAltitudeAvailable.expire(Time, fixed(30));
  TotalEnergyVarioAvailable.expire(Time, fixed(5));
  NettoVarioAvailable.expire(Time, fixed(5));
  ExternalWindAvailable.expire(Time, fixed(600));
  WindAvailable.expire(Time, fixed(600));
}

void
NMEA_INFO::complement(const NMEA_INFO &add)
{
  gps.complement(add.gps);

  acceleration.complement(add.acceleration);

  /* calculated: flight */

  if (add.gps.Connected > gps.Connected) {
    gps = add.gps;
    Location = add.Location;
    TrackBearing = add.TrackBearing;
    GroundSpeed = add.GroundSpeed;
    GPSAltitude = add.GPSAltitude;
    Time = add.Time;
    DateTime = add.DateTime;
  }

  if (AirspeedAvailable.complement(add.AirspeedAvailable)) {
    TrueAirspeed = add.TrueAirspeed;
    IndicatedAirspeed = add.IndicatedAirspeed;
  }

  /* calculated: Heading, TurnRateWind, TurnRate */
  /* calculated: TrueAirspeedEstimated */

  if ((BaroAltitudeAvailable.complement(add.BaroAltitudeAvailable) ||
       (BaroAltitudeOrigin <= BARO_ALTITUDE_UNKNOWN &&
        add.BaroAltitudeOrigin > BaroAltitudeOrigin)) &&
      add.BaroAltitudeAvailable) {
    BaroAltitude = add.BaroAltitude;
    BaroAltitudeOrigin = add.BaroAltitudeOrigin;
    BaroAltitudeAvailable = add.BaroAltitudeAvailable;
  }

  /* calculated: EnergyHeight, TEAltitude, working_band_height,
     NavAltitude,working_band_fraction, AltitudeAGL */

  /* managed by DeviceBlackboard: pressure */

  /* calculated: GliderSinkRate, GPSVario, GPSVarioTE, BruttoVario */

  if (TotalEnergyVarioAvailable.complement(add.TotalEnergyVarioAvailable))
    TotalEnergyVario = add.TotalEnergyVario;

  if (NettoVarioAvailable.complement(add.NettoVarioAvailable))
    NettoVario = add.NettoVario;

  // XXX MacCready, Ballast, Bugs

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

  if (!positive(SupplyBatteryVoltage) && positive(add.SupplyBatteryVoltage))
    SupplyBatteryVoltage = add.SupplyBatteryVoltage;

  if (!SwitchStateAvailable && add.SwitchStateAvailable)
    SwitchState = add.SwitchState;

  flarm.complement(add.flarm);
}

const AIRCRAFT_STATE
ToAircraftState(const NMEA_INFO &info)
{
  AIRCRAFT_STATE aircraft;

  /* SPEED_STATE */
  aircraft.Speed = info.GroundSpeed;
  aircraft.TrueAirspeed = info.TrueAirspeed;
  aircraft.IndicatedAirspeed = info.IndicatedAirspeed;

  /* ALTITUDE_STATE */
  aircraft.NavAltitude = info.NavAltitude;
  aircraft.working_band_fraction = info.working_band_fraction;
  aircraft.AltitudeAGL = info.AltitudeAGL;
  aircraft.AirspaceAltitude = info.GetAltitudeBaroPreferred();

  /* VARIO_INFO */
  aircraft.Vario = info.BruttoVario;
  aircraft.NettoVario = info.NettoVario;

  /* FLYING_STATE */
  (FLYING_STATE &)aircraft = info.flight;

  /* AIRCRAFT_STATE */
  aircraft.Time = info.Time;
  aircraft.Location = info.Location;
  aircraft.TrackBearing = info.TrackBearing;
  aircraft.Gload = info.acceleration.Gload;
  aircraft.wind = info.wind;

  return aircraft;
}
