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

#include "DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"
#include "Math/Geometry.hpp"
#include "TeamCodeCalculation.h"
#include "UtilsFLARM.hpp"
#include "Asset.hpp"
#if defined(_SIM_) && !defined(NDEBUG)
#include "Device/Parser.h"
#endif
#include "WayPoint.hpp"

DeviceBlackboard device_blackboard;

/**
 * Initializes the DeviceBlackboard
 */
void
DeviceBlackboard::Initialise()
{
  ScopeLock protect(mutexBlackboard);

  // Clear the gps_info and calculated_info
  memset( &gps_info, 0, sizeof(NMEA_INFO));
  memset( &calculated_info, 0, sizeof(DERIVED_INFO));

  // Set the NAVWarning positive (assume not gps found yet)
  gps_info.NAVWarning = true;

  // Clear the SwitchStates
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

  // Set GPS assumed time to system time
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

/**
 * Sets the location and altitude to loc and alt
 *
 * Called at startup when no gps data available yet
 * @param loc New location
 * @param alt New altitude
 */
void
DeviceBlackboard::SetStartupLocation(const GEOPOINT &loc,
				     const double alt)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().Location = loc;
  SetBasic().Altitude = alt;
}

/**
 * Sets the location, altitude and other basic parameters
 *
 * Used by the ReplayLogger
 * @param loc New location
 * @param speed New speed
 * @param bearing New bearing
 * @param alt New altitude
 * @param baroalt New barometric altitude
 * @param t New time
 * @see ReplayLogger::UpdateInternal()
 */
void
DeviceBlackboard::SetLocation(const GEOPOINT &loc,
			      const double speed, const double bearing,
			      const double alt, const double baroalt, const double t)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().Location = loc;
  SetBasic().Speed = speed;
  SetBasic().IndicatedAirspeed = speed; // cheat
  SetBasic().TrackBearing = bearing;
  SetBasic().Altitude = alt;
  SetBasic().BaroAltitude = baroalt;
  SetBasic().Time = t;
  SetBasic().Replay = true;
};

/**
 * Stops the replay
 */
void DeviceBlackboard::StopReplay() {
  ScopeLock protect(mutexBlackboard);
  SetBasic().Speed = 0;
  SetBasic().Replay = false;
}

/**
 * Sets the NAVWarning to val
 * @param val New value for NAVWarning
 */
void
DeviceBlackboard::SetNAVWarning(bool val)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().NAVWarning = val;
  if (!val) {
    // QUESTION TB: what going on here?
    // externally forced
    SetBasic().SatellitesUsed = 6;
  }
}

/**
 * Lowers the connection status of the device
 *
 * Connected + Fix -> Connected + No Fix
 * Connected + No Fix -> Not connected
 * @return True if still connected afterwards, False otherwise
 */
bool
DeviceBlackboard::LowerConnection()
{
  ScopeLock protect(mutexBlackboard);
  bool retval;
  if (Basic().Connected) {
    SetBasic().Connected--;
  }
  if (Basic().Connected) {
    retval = true;
  } else {
    retval = false;
  }
  return retval;
}

/**
 * Raises the connection status to connected + fix
 */
void
DeviceBlackboard::RaiseConnection()
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().Connected = 2;
}

#ifdef _SIM_
void
DeviceBlackboard::ProcessSimulation()
{
  ScopeLock protect(mutexBlackboard);
  SetNAVWarning(false);
  FindLatitudeLongitude(Basic().Location,
			Basic().TrackBearing,
			Basic().Speed*1.0,
			&SetBasic().Location);
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
}
#endif /* _SIM_ */

/**
 * Sets the GPS speed and indicated airspeed to val
 *
 * not in use
 * @param val New speed
 */
void
DeviceBlackboard::SetSpeed(double val)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().Speed = val;
  SetBasic().IndicatedAirspeed = val;
}

/**
 * Sets the TrackBearing to val
 *
 * not in use
 * @param val New TrackBearing
 */
void
DeviceBlackboard::SetTrackBearing(double val)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().TrackBearing = AngleLimit360(val);
}

/**
 * Sets the altitude and barometric altitude to val
 *
 * not in use
 * @param val New altitude
 */
void
DeviceBlackboard::SetAltitude(double val)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().Altitude = val;
  SetBasic().BaroAltitude = val;
}

/**
 * Reads the given derived_info usually provided by the
 * GlideComputerBlackboard and saves it to the own Blackboard
 * @param derived_info Calculated information usually provided
 * by the GlideComputerBlackboard
 */
void
DeviceBlackboard::ReadBlackboard(const DERIVED_INFO &derived_info)
{
  memcpy(&calculated_info,&derived_info,sizeof(DERIVED_INFO));
}

/**
 * Reads the given settings usually provided by the InterfaceBlackboard
 * and saves it to the own Blackboard
 * @param settings SettingsComputer usually provided by the
 * InterfaceBlackboard
 */
void
DeviceBlackboard::ReadSettingsComputer(const SETTINGS_COMPUTER
					      &settings)
{
  memcpy(&settings_computer,&settings,sizeof(SETTINGS_COMPUTER));
}

/**
 * Reads the given settings usually provided by the InterfaceBlackboard
 * and saves it to the own Blackboard
 * @param settings SettingsMap usually provided by the
 * InterfaceBlackboard
 */
void
DeviceBlackboard::ReadSettingsMap(const SETTINGS_MAP
				  &settings)
{
  memcpy(&settings_map,&settings,sizeof(SETTINGS_MAP));
}

/**
 * Checks for timeout of the FLARM targets and
 * saves the status to Basic()
 */
void
DeviceBlackboard::FLARM_RefreshSlots() {
  int i;
  bool present = false;

  // if (Flarm data is available)
  if (Basic().FLARM_Available) {
    // for each item in FLARM_Traffic
    for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
      // if (FLARM_Traffic[i] has data)
      if (Basic().FLARM_Traffic[i].ID>0) {
        // if (FLARM target is too old or time has gone backwards)
        if ((Basic().Time > Basic().FLARM_Traffic[i].Time_Fix + 2)
            || (Basic().Time < Basic().FLARM_Traffic[i].Time_Fix)) {
          // clear this slot if it is too old (2 seconds), or if
          // time has gone backwards (due to replay)
          SetBasic().FLARM_Traffic[i].ID = 0;
          SetBasic().FLARM_Traffic[i].Name[0] = 0;
        } else {
          // FLARM data is present
          present = true;
        }
      }
    }
  }

  // Save the status of present into Basic()
  SetBasic().FLARMTraffic = present;
  SetBasic().NewTraffic = false;
}

/**
 * Sets the system time to GPS time if not yet done and
 * defined in settings
 */
void
DeviceBlackboard::SetSystemTime() {
  // TODO JMW: this should be done outside the parser..
  if (is_simulator())
    return;

  // Altair doesn't have a battery-backed up realtime clock,
  // so as soon as we get a fix for the first time, set the
  // system clock to the GPS time.
  static bool sysTimeInitialised = false;

  if (!Basic().NAVWarning && SettingsMap().SetSystemTimeFromGPS
      && !sysTimeInitialised) {
    SYSTEMTIME sysTime;
    ::GetSystemTime(&sysTime);

    // QUESTION TB: why do we save hours, mins and secs?!
    int hours = (int)Basic().Hour;
    int mins = (int)Basic().Minute;
    int secs = (int)Basic().Second;
    sysTime.wYear = (unsigned short)Basic().Year;
    sysTime.wMonth = (unsigned short)Basic().Month;
    sysTime.wDay = (unsigned short)Basic().Day;
    sysTime.wHour = (unsigned short)hours;
    sysTime.wMinute = (unsigned short)mins;
    sysTime.wSecond = (unsigned short)secs;
    sysTime.wMilliseconds = 0;
    sysTimeInitialised = (::SetSystemTime(&sysTime)==true);

#if defined(GNAV) && !defined(WINDOWSPC)
    TIME_ZONE_INFORMATION tzi;
    tzi.Bias = -SettingsComputer().UTCOffset/60;
    _tcscpy(tzi.StandardName,TEXT("Altair"));
    tzi.StandardDate.wMonth= 0; // disable daylight savings
    tzi.StandardBias = 0;
    _tcscpy(tzi.DaylightName,TEXT("Altair"));
    tzi.DaylightDate.wMonth= 0; // disable daylight savings
    tzi.DaylightBias = 0;

    SetTimeZoneInformation(&tzi);
#endif
    sysTimeInitialised =true;
  }
}

/**
 * Tries to find a name for every current FLARM_Traffic id
 */
void
DeviceBlackboard::FLARM_ScanTraffic()
{
  // TODO: this is a bit silly, it searches every time a target is
  // visible... going to be slow..
  // should only scan the first time it appears with that ID.
  // at least it is now not being done by the parser

  // if (FLARM data is available)
  if (Basic().FLARM_Available) {
    // for each item in FLARM_Traffic
    for (int flarm_slot=0; flarm_slot<FLARM_MAX_TRAFFIC; flarm_slot++) {
      // if (FLARM_Traffic[flarm_slot] has data)
      if (Basic().FLARM_Traffic[flarm_slot].ID>0) {
        // if (Target currently without name)
        if (!_tcslen(Basic().FLARM_Traffic[flarm_slot].Name)) {
          // need to lookup name for this target
	        const TCHAR *fname =
              LookupFLARMDetails(Basic().FLARM_Traffic[flarm_slot].ID);

          // if (name for FLARM id found)
          if (fname) {
            _tcscpy(SetBasic().FLARM_Traffic[flarm_slot].Name, fname);
          } else {
            SetBasic().FLARM_Traffic[flarm_slot].Name[0] = 0;
          }
        }
      }
    }
  }
}
