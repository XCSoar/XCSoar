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


#include "Blackboard.hpp"
#include "Protection.hpp"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"
#include "Math/Geometry.hpp"

#if defined(_SIM_) && !defined(NDEBUG)
#include "Device/Parser.h"
#endif

DeviceBlackboard device_blackboard;
InterfaceBlackboard InstrumentBlackboard::blackboard;

void 
DeviceBlackboard::Initialise() 
{
  memset( &gps_info, 0, sizeof(NMEA_INFO));
  memset( &calculated_info, 0, sizeof(DERIVED_INFO));

  gps_info.NAVWarning = true; // default, no gps at all!
  gps_info.SwitchState.AirbrakeLocked = false;
  gps_info.SwitchState.FlapPositive = false;
  gps_info.SwitchState.FlapNeutral = false;
  gps_info.SwitchState.FlapNegative = false;
  gps_info.SwitchState.GearExtended = false;
  gps_info.SwitchState.Acknowledge = false;
  gps_info.SwitchState.Repeat = false;
  gps_info.SwitchState.SpeedCommand = false;
  gps_info.SwitchState.UserSwitchUp = false;
  gps_info.SwitchState.UserSwitchMiddle = false;
  gps_info.SwitchState.UserSwitchDown = false;
  gps_info.SwitchState.VarioCircling = false;

  SYSTEMTIME pda_time;
  GetSystemTime(&pda_time);
  gps_info.Time  = pda_time.wHour*3600+pda_time.wMinute*60+pda_time.wSecond;
  gps_info.Year  = pda_time.wYear;
  gps_info.Month = pda_time.wMonth;
  gps_info.Day	 = pda_time.wDay;
  gps_info.Hour  = pda_time.wHour;
  gps_info.Minute = pda_time.wMinute;
  gps_info.Second = pda_time.wSecond;

#ifdef _SIM_
  #ifdef _SIM_STARTUPSPEED
  gps_info.Speed = _SIM_STARTUPSPEED;
  #endif
  #ifdef _SIM_STARTUPALTITUDE
  gps_info.Altitude = _SIM_STARTUPALTITUDE;
  #endif
#endif
}

void 
DeviceBlackboard::SetStartupLocation(double lon, double lat, 
				     double alt) 
{
  mutexFlightData.Lock();
  SetBasic().Longitude = lon;
  SetBasic().Latitude = lat;
  SetBasic().Altitude = alt;
  mutexFlightData.Unlock();
}

// used by replay logger
void 
DeviceBlackboard::SetLocation(double lon, double lat, 
			      double speed, double bearing,
			      double alt, double baroalt, double t) 
{
  mutexFlightData.Lock();
  SetBasic().Longitude = lon;
  SetBasic().Latitude = lat;
  SetBasic().Speed = speed;
  SetBasic().IndicatedAirspeed = speed; // cheat
  SetBasic().TrackBearing = bearing;
  SetBasic().Altitude = alt;
  SetBasic().BaroAltitude = baroalt;
  SetBasic().Time = t;
  mutexFlightData.Unlock();
};

void DeviceBlackboard::StopReplay() {
  mutexFlightData.Lock();
  SetBasic().Speed = 0;
  mutexFlightData.Unlock();
}


//////////////////

void 
MapWindowBlackboard::ReadSettingsComputer(const SETTINGS_COMPUTER 
					      &settings) 
{
  memcpy(&settings_computer,&settings,sizeof(SETTINGS_COMPUTER));
}

void 
MapWindowBlackboard::ReadSettingsMap(const SETTINGS_MAP 
				     &settings) 
{
  memcpy(&settings_map,&settings,sizeof(SETTINGS_MAP));
}

void 
MapWindowBlackboard::ReadBlackboard(const NMEA_INFO &nmea_info,
				    const DERIVED_INFO &derived_info) 
{
  memcpy(&gps_info,&nmea_info,sizeof(NMEA_INFO));
  memcpy(&calculated_info,&derived_info,sizeof(DERIVED_INFO));
}


void 
GlideComputerBlackboard::ReadBlackboard(const NMEA_INFO &nmea_info) 
{
  _time_retreated = false;
  if (nmea_info.Time< gps_info.Time) {
    // backwards in time, so reset last
    memcpy(&last_gps_info,&nmea_info,sizeof(NMEA_INFO));
    memcpy(&last_calculated_info,&calculated_info,sizeof(DERIVED_INFO));
    _time_retreated = true;
  } else if (nmea_info.Time> gps_info.Time) {
    // forwards in time, so save state
    memcpy(&last_gps_info,&gps_info,sizeof(NMEA_INFO));
    memcpy(&last_calculated_info,&calculated_info,sizeof(DERIVED_INFO));
  }
  memcpy(&gps_info,&nmea_info,sizeof(NMEA_INFO));
  // if time hasn't advanced, don't copy last calculated
}

void 
GlideComputerBlackboard::ReadSettingsComputer(const SETTINGS_COMPUTER 
					      &settings) 
{
  memcpy(&settings_computer,&settings,sizeof(SETTINGS_COMPUTER));
}

//////


void 
InterfaceBlackboard::ReadBlackboardCalculated(const DERIVED_INFO &derived_info)
{
  memcpy(&calculated_info,&derived_info,sizeof(DERIVED_INFO));
}

void 
InterfaceBlackboard::ReadBlackboardBasic(const NMEA_INFO &nmea_info)
{
  memcpy(&gps_info,&nmea_info,sizeof(NMEA_INFO));
}

void 
InterfaceBlackboard::ReadSettingsComputer(const SETTINGS_COMPUTER 
					  &settings) 
{
  memcpy(&settings_computer,&settings,sizeof(SETTINGS_COMPUTER));
}


///////

void 
DeviceBlackboard::SetNAVWarning(bool val)
{
  mutexFlightData.Lock();
  SetBasic().NAVWarning = val;
  if (!val) {
    // externally forced
    SetBasic().SatellitesUsed = 6;
  }
  mutexFlightData.Unlock();
}

bool
DeviceBlackboard::LowerConnection()
{
  bool retval;
  mutexFlightData.Lock();
  if (Basic().Connected) {
    SetBasic().Connected--;
  }
  if (Basic().Connected) {
    retval = true;
  } else {
    retval = false;
  }
  mutexFlightData.Unlock();
  return retval;
}

void
DeviceBlackboard::RaiseConnection()
{
  mutexFlightData.Lock();
  SetBasic().Connected = 2;
  mutexFlightData.Unlock();
}

#ifdef _SIM_
void
DeviceBlackboard::ProcessSimulation()
{
  SetNAVWarning(false);
  mutexFlightData.Lock();
  FindLatitudeLongitude(Basic().Latitude, 
			Basic().Longitude,
			Basic().TrackBearing, 
			Basic().Speed*1.0,
			&SetBasic().Latitude,
			&SetBasic().Longitude);
  SetBasic().Time+= 1.0;
  long tsec = (long)Basic().Time;
  SetBasic().Hour = tsec/3600;
  SetBasic().Minute = (tsec-Basic().Hour*3600)/60;
  SetBasic().Second = (tsec-Basic().Hour*3600-Basic().Minute*60);

#ifndef NDEBUG
  // use this to test FLARM parsing/display
#ifndef GNAV
  NMEAParser::TestRoutine(&SetBasic());
#endif
#endif

  mutexFlightData.Unlock();
}
#endif /* _SIM_ */

void
DeviceBlackboard::SetSpeed(double val)
{
  mutexFlightData.Lock();
  SetBasic().Speed = val;
  SetBasic().IndicatedAirspeed = val;
  mutexFlightData.Unlock();
}

void
DeviceBlackboard::SetTrackBearing(double val)
{
  mutexFlightData.Lock();
  SetBasic().TrackBearing = AngleLimit360(val);
  mutexFlightData.Unlock();
}

void
DeviceBlackboard::SetAltitude(double val)
{
  mutexFlightData.Lock();
  SetBasic().Altitude = val;
  SetBasic().BaroAltitude = val;
  mutexFlightData.Unlock();
}

void 
DeviceBlackboard::ReadBlackboard(const DERIVED_INFO &derived_info)
{
  memcpy(&calculated_info,&derived_info,sizeof(DERIVED_INFO));
}

void 
DeviceBlackboard::ReadSettingsComputer(const SETTINGS_COMPUTER 
					      &settings) 
{
  memcpy(&settings_computer,&settings,sizeof(SETTINGS_COMPUTER));
}

void 
DeviceBlackboard::ReadSettingsMap(const SETTINGS_MAP 
				  &settings) 
{
  memcpy(&settings_map,&settings,sizeof(SETTINGS_MAP));
}


