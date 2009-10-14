/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "GeoPoint.hpp"
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

struct NMEA_INFO
{
  unsigned Connected; /**< Is a GPS unit connected? */
  GEOPOINT Location; /**< Current Location (lat/lon) */
  double TrackBearing; /**< Track angle in degrees */
  double Speed; /**< Speed over ground in m/s */
  double Altitude; /**< GPS Altitude */
  //  TCHAR  WaypointID[WAY_POINT_ID_SIZE + 1];
  //  double WaypointBearing;
  //  double WaypointDistance;
  //  double WaypointSpeed; IGNORED NOW
  double CrossTrackError; /**< not in use(?) */
  double Time; /**< GPS time */
  int Hour; /**< GPS time (hours) */
  int Minute; /**< GPS time (minutes) */
  int Second; /**< GPS time (seconds) */
  int Month; /**< GPS date (month) */
  int Day; /**< GPS date (day) */
  int Year; /**< GPS date (year) */
  int NAVWarning; /**< GPS fix not valid */
  double IndicatedAirspeed; /**< Indicated air speed (if available) @see TrueAirspeed @see AirDensityRatio */
  double TrueAirspeed; /**< True air speed (if available) @see IndicatedAirspeed */
  double BaroAltitude; /**< Barometric altitude (if available) @see BaroAltitudeAvailable @see Altitude */
  double MacReady; /**< MacCready value of external device (if available) */
  bool BaroAltitudeAvailable; /**< Is a barometric altitude available? @see BaroAltitude */
  bool ExternalWindAvailalbe; /**< Is external wind information available? @see ExternalWindSpeed @see ExternalWindDirection */
  double ExternalWindSpeed; /**< Wind speed of external device (if available) @see ExternalWindDirection @see ExternalWindAvailalbe */
  double ExternalWindDirection; /**< Wind direction of external device (if available) @see ExternalWindSpeed @see ExternalWindAvailalbe */
  bool VarioAvailable; /**< Is an external vario signal available? @see Vario */
  bool NettoVarioAvailable; /**< Is an external netto vario signal available? @see NettoVario */
  bool AirspeedAvailable; /**< Is air speed information available? @see TrueAirspeed */
  double Vario; /**< Vario signal of external device (if available) @see VarioAvailable */
  double NettoVario; /**< Netto vario signal of external device (if available) @see NettoVarioAvailable */
  double Ballast; /**< Ballast information of external device (if available) */
  double Bugs; /**< Bugs information of external device (if available) */
  double Gload; /**< G-Load information of external device (if available) @see AccelerationAvailable */
  bool AccelerationAvailable; /**< Is G-load information available? @see Gload @see AccelX @see AccelY */
  double AccelX; /**< G-Load information of external device in X-direction (if available) @see AccelerationAvailable */
  double AccelZ; /**< G-Load information of external device in Y-direction (if available) @see AccelerationAvailable */
  int SatellitesUsed; /**< Number of satellites used for gps fix */
  bool TemperatureAvailable; /**< Is temperature information available? @see OutsideAirTemperatur */
  double OutsideAirTemperature; /**< Temperature of outside air (if available) @see TemperatureAvailable */
  bool HumidityAvailable; /**< Is humidity information available? @see RelativeHumidity */
  double RelativeHumidity; /**< Humidity of outside air (if available) @see HumidityAvailable */

  unsigned short FLARM_RX; /**< Number of received FLARM devices */
  unsigned short FLARM_TX; /**< Transmit status */
  unsigned short FLARM_GPS; /**< GPS status */
  unsigned short FLARM_AlarmLevel; /**< Alarm level of FLARM (0-3) */
  bool FLARM_Available; /**< Is FLARM information available? */
  FLARM_TRAFFIC FLARM_Traffic[FLARM_MAX_TRAFFIC]; /**< Flarm traffic information */
  bool FLARMTraffic; /**< Is there FLARM traffic present? @see FLARM_Traffic */
  bool NewTraffic; /**< Is there new FLARM traffic present? @see FLARM_Traffic */
  int SatelliteIDs[MAXSATELLITES]; /**< GPS Satellite information */

  double SupplyBatteryVoltage; /**< Battery supply voltage information (if available) */

  SWITCH_INFO SwitchState; /**< Switch state of the user inputs */

  bool MovementDetected; /**< Is the GPS unit moving? (Speed > 2.0) */

  double StallRatio;
  bool Replay; /**< Is XCSoar in replay mode? */
};

#endif
