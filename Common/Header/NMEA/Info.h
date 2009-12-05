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

#include "Navigation/GeoPoint.hpp"
#include "Navigation/Aircraft.hpp"
#include "FLARM/Traffic.h"
#include "Sizes.h"

struct SWITCH_INFO
{
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

/**
 * A struct that holds all the parsed data read from the connected devices
 */
struct NMEA_INFO: public AIRCRAFT_STATE
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

  //############
  //   Speeds
  //############

  /**
   * Is air speed information available?
   * @see TrueAirspeed
   */
  bool AirspeedAvailable;
  /**
   * Indicated air speed (if available)
   * @see Speed
   * @see TrueAirspeed
   * @see AirDensityRatio
   */
  double IndicatedAirspeed;
  /**
   * True air speed (if available)
   * @see Speed
   * @see IndicatedAirspeed
   */
  double TrueAirspeed;

  //##############
  //   Altitude
  //##############

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

  //##########
  //   Time
  //##########

  /** GPS time (hours) */
  int Hour;
  /** GPS time (minutes) */
  int Minute;
  /**< GPS time (seconds) */
  int Second;
  /**< GPS date (month) */
  int Month;
  /**< GPS date (day) */
  int Day;
  /**< GPS date (year) */
  int Year;

  //###########
  //   Vario
  //###########

  /**
   * Is an external vario signal available?
   * @see Vario
   */
  bool VarioAvailable;
  /**
   * Vario signal of external device (if available)
   * @see VarioAvailable
   */
  double Vario;

  /**
   * Is an external netto vario signal available?
   * @see NettoVario
   */
  bool NettoVarioAvailable;
  /**
   * Netto vario signal of external device (if available)
   * @see NettoVarioAvailable
   */
  double NettoVario;

  //##############
  //   Settings
  //##############

  /** MacCready value of external device (if available) */
  double MacReady;

  /** Ballast information of external device (if available) */
  double Ballast;

  /** Bugs information of external device (if available) */
  double Bugs;

  //##################
  //   Acceleration
  //##################

  /**
   * Is G-load information available?
   * @see Gload
   * @see AccelX
   * @see AccelY
   */
  bool AccelerationAvailable;
  /**
   * G-Load information of external device (if available)
   * @see AccelerationAvailable
   */
  double Gload;
  /**
   * G-Load information of external device in X-direction (if available)
   * @see AccelerationAvailable
   */
  double AccelX;
  /**
   * G-Load information of external device in Y-direction (if available)
   * @see AccelerationAvailable
   */
  double AccelZ;

  //################
  //   Atmosphere
  //################

  /**
   * Is external wind information available?
   * @see ExternalWindSpeed
   * @see ExternalWindDirection
   */
  bool ExternalWindAvailalbe;
  /**
   * Wind speed of external device (if available)
   * @see ExternalWindDirection
   * @see ExternalWindAvailalbe
   */
  double ExternalWindSpeed;
  /**
   * Wind direction of external device (if available)
   * @see ExternalWindSpeed
   * @see ExternalWindAvailalbe
   */
  double ExternalWindDirection;

  /**
   * Is temperature information available?
   * @see OutsideAirTemperatur
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
  //   FLARM
  //###########

  /** Number of received FLARM devices */
  unsigned short FLARM_RX;
  /** Transmit status */
  unsigned short FLARM_TX;
  /** GPS status */
  unsigned short FLARM_GPS;
  /** Alarm level of FLARM (0-3) */
  unsigned short FLARM_AlarmLevel;
  /** Is FLARM information available? */
  bool FLARM_Available;
  /** Flarm traffic information */
  FLARM_TRAFFIC FLARM_Traffic[FLARM_MAX_TRAFFIC];
  /**
   * Is there FLARM traffic present?
   * @see FLARM_Traffic
   */
  bool FLARMTraffic;
  /**
   * Is there new FLARM traffic present?
   * @see FLARM_Traffic
   */
  bool NewTraffic;

  //###########
  //   Other
  //###########

  /** Battery supply voltage information (if available) */
  double SupplyBatteryVoltage;

  /** Switch state of the user inputs */
  SWITCH_INFO SwitchState;

  double StallRatio;

  /** Is XCSoar in replay mode? */
  bool Replay;

  //  TCHAR  WaypointID[WAY_POINT_ID_SIZE + 1];
  //  double WaypointBearing;
  //  double WaypointDistance;
  //  double WaypointSpeed; IGNORED NOW

  /**
   * Returns the barometric altitude, and falls back to the GPS
   * altitude.
   */
  fixed GetAnyAltitude() const {
    return BaroAltitudeAvailable
      ? BaroAltitude
      : Altitude;
  }
};

#endif
