/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_NMEA_INFO_H
#define XCSOAR_NMEA_INFO_H

#include "DateTime.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Navigation/Aircraft.hpp"
#include "Atmosphere/Pressure.hpp"
#include "FLARM/State.hpp"
#include "Sizes.h"

struct SWITCH_INFO
{
  bool Available;

  bool AirbrakeLocked;
  bool FlapPositive;
  bool FlapNeutral;
  bool FlapNegative;
  bool GearExtended;
  bool Acknowledge;
  bool Repeat;
  bool SpeedCommand;
  bool UserSwitchUp;
  bool UserSwitchMiddle;
  bool UserSwitchDown;
  bool VarioCircling;
  bool FlapLanding;
  // bool Stall;
};

struct GPS_STATE
{
  //############
  //   Status
  //############

  /**
   * Is a GPS unit connected?
   *
   * 0 = not connected
   * 1 = connected, waiting for fix
   * 2 = connected, fix found
   */
  unsigned Connected;

  /** GPS fix not valid */
  int NAVWarning;

  /** Number of satellites used for gps fix */
  int SatellitesUsed;

  /** GPS Satellite ids */
  int SatelliteIDs[MAXSATELLITES];

  /** Is the GPS unit moving? (Speed > 2.0) */
  bool MovementDetected;

  /** Is XCSoar in replay mode? */
  bool Replay;

  /**
   * Did the simulator provide the GPS position?
   */
  bool Simulator;
};

struct ACCELERATION_STATE
{
  //##################
  //   Acceleration
  //##################

  /** Estimated bank angle */
  fixed BankAngle;
  /** Estimated pitch angle */
  fixed PitchAngle;

  /**
   * Is G-load information available?
   * @see Gload
   */
  bool Available;

  /**
   * G-Load information of external device (if available)
   * or estimated (assuming balanced turn) 
   * @see AccelerationAvailable
   */
  fixed Gload;
};


/**
 * A struct that holds all the parsed data read from the connected devices
 */
struct NMEA_INFO {
  GPS_STATE gps;

  ACCELERATION_STATE acceleration;

  FLYING_STATE flight;

  /** location of aircraft */
  GEOPOINT Location;

  /** track angle in degrees true */
  fixed TrackBearing;

  /** Bearing including wind factor */
  fixed Heading;

  /** Turn rate based on heading (including wind) */
  fixed TurnRateWind;

  /** Turn rate based on track */
  fixed TurnRate;

  /** Estimated track bearing at next time step @author JMW */
  fixed NextTrackBearing;

  //############
  //   Speeds
  //############

  /**
   * Is air speed information available?
   * If not, will be estimated from ground speed and wind estimate
   * @see TrueAirspeed in Aircraft
   */
  bool AirspeedAvailable;

  /**
   * Speed over ground in m/s
   * @see TrueAirspeed
   * @see IndicatedAirspeed
   */
  fixed GroundSpeed; 

  /**
   * True air speed (m/s)
   * @see Speed
   * @see IndicatedAirspeed
   */
  fixed TrueAirspeed;

  /**
   * Indicated air speed (m/s)
   * @see Speed
   * @see TrueAirspeed
   * @see AirDensityRatio
   */
  fixed IndicatedAirspeed;

  fixed TrueAirspeedEstimated;

  //##############
  //   Altitude
  //##############

  fixed GPSAltitude; /**< GPS altitude AMSL (m) */

  /**
   * Is a barometric altitude available?
   * @see BaroAltitude
   */
  bool BaroAltitudeAvailable;
  /**
   * Barometric altitude (if available)
   * @see BaroAltitudeAvailable
   * @see Altitude
   */
  fixed BaroAltitude;

  /** Energy height excess to slow to best glide speed @author JMW */
  fixed EnergyHeight;

  /** Nav Altitude + Energy height (m) */
  fixed TEAltitude;

  /** Height above working band/safety (m) */
  fixed working_band_height;

  /** Altitude used for navigation (GPS or Baro) */
  fixed NavAltitude;

  /** Fraction of working band height */
  fixed working_band_fraction;

  /** Altitude over terrain */
  fixed AltitudeAGL;

  /**
   * Troposhere atmosphere model for QNH correction
   */
  AtmosphericPressure pressure;

  //##########
  //   Time
  //##########

  fixed Time; /**< global time (seconds UTC) */

  /** GPS date and time */
  BrokenDateTime DateTime;

  //###########
  //   Vario
  //###########

  fixed GliderSinkRate;

  /** GPS-based vario */
  fixed GPSVario;
  /** GPS-based vario including energy height */
  fixed GPSVarioTE;

  /**
   * Is an external vario signal available?
   * @see TotalEnergyVario
   */
  bool TotalEnergyVarioAvailable;

  /**
   * Is an external netto vario signal available?
   * @see NettoVario
   */
  bool NettoVarioAvailable;

  /**
   * Rate of change of total energy of aircraft (m/s, up positive)
   * @see TotalEnergyVarioAvailable
   */
  fixed TotalEnergyVario;

  /**
   * Vertical speed of air mass (m/s, up positive)
   * @see NettoVarioAvailable
   */
  fixed NettoVario;

  //##############
  //   Settings
  //##############

  /** MacCready value of external device (if available) */
  double MacCready;

  /** Ballast information of external device (if available) */
  double Ballast;

  /** Bugs information of external device (if available) */
  double Bugs;

  //################
  //   Atmosphere
  //################

  /**
   * Is external wind information available?
   * @see ExternalWindSpeed
   * @see ExternalWindDirection
   */
  bool ExternalWindAvailable;

  SpeedVector wind;

  /**
   * Is temperature information available?
   * @see OutsideAirTemperature
   */
  bool TemperatureAvailable;
  /**
   * Temperature of outside air (if available)
   * @see TemperatureAvailable
   */
  double OutsideAirTemperature;

  /**
   * Is humidity information available?
   * @see RelativeHumidity
   */
  bool HumidityAvailable;
  /**
   * Humidity of outside air (if available)
   * @see HumidityAvailable
   */
  double RelativeHumidity;

  //###########
  //   Other
  //###########

  /** Battery supply voltage information (if available) */
  double SupplyBatteryVoltage;

  /** Switch state of the user inputs */
  SWITCH_INFO SwitchState;

  double StallRatio;

  FLARM_STATE flarm;

  /**
   * Returns the barometric altitude, and falls back to the GPS
   * altitude.
   */
  fixed GetAltitudeBaroPreferred() const {
    return BaroAltitudeAvailable
      ? BaroAltitude
      : GPSAltitude;
  }
};

static inline const AIRCRAFT_STATE
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

  /* VARIO_INFO */
  aircraft.Vario = info.TotalEnergyVario;
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

#endif
