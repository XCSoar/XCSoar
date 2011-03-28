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

#include "DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"
#include "TeamCodeCalculation.h"
#include "FLARM/FlarmDetails.hpp"
#include "Asset.hpp"
#include "Device/Parser.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Device/All.hpp"
#include "Math/Constants.h"
#include "GlideSolvers/GlidePolar.hpp"
#include "Simulator.hpp"
#include "OS/Clock.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include <limits.h>

#ifdef WIN32
#include <windows.h>
#endif

#define fixed_inv_g fixed(1.0/9.81)
#define fixed_small fixed(0.001)

DeviceBlackboard device_blackboard;

/**
 * Initializes the DeviceBlackboard
 */
void
DeviceBlackboard::Initialise()
{
  ScopeLock protect(mutexBlackboard);

  // Clear the gps_info and calculated_info
  gps_info.reset();
  calculated_info.reset();

  // Set GPS assumed time to system time
  gps_info.DateTime = BrokenDateTime::NowUTC();
  gps_info.Time = fixed(gps_info.DateTime.hour * 3600 +
                        gps_info.DateTime.minute * 60 +
                        gps_info.DateTime.second);
}

/**
 * Sets the location and altitude to loc and alt
 *
 * Called at startup when no gps data available yet
 * @param loc New location
 * @param alt New altitude
 */
void
DeviceBlackboard::SetStartupLocation(const GeoPoint &loc, const fixed alt)
{
  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();

  basic.Location = loc;
  basic.GPSAltitude = alt;

  // set NAVWarning flags because this value was not provided
  // by a real GPS
  basic.LocationAvailable.clear();
  basic.GPSAltitudeAvailable.clear();
}

/**
 * Sets the location, altitude and other basic parameters
 *
 * Used by the IgcReplay
 * @param loc New location
 * @param speed New speed
 * @param bearing New bearing
 * @param alt New altitude
 * @param baroalt New barometric altitude
 * @param t New time
 * @see IgcReplay::UpdateInternal()
 */
void
DeviceBlackboard::SetLocation(const GeoPoint &loc,
                              const fixed speed, const Angle bearing,
                              const fixed alt, const fixed baroalt,
                              const fixed t)
{
  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();

  basic.Connected.update(fixed(MonotonicClockMS()) / 1000);
  basic.gps.SatellitesUsed = 6;
  basic.acceleration.Available = false;
  basic.Location = loc;
  basic.LocationAvailable.update(t);
  basic.GroundSpeed = speed;
  basic.GroundSpeedAvailable.update(t);
  basic.AirspeedAvailable.clear(); // Clear airspeed as it is not given by any value.
  basic.TrackBearing = bearing;
  basic.TrackBearingAvailable.update(t);
  basic.GPSAltitude = alt;
  basic.GPSAltitudeAvailable.update(t);
  basic.ProvidePressureAltitude(NMEA_INFO::BARO_ALTITUDE_UNKNOWN, baroalt);
  basic.Time = t;
  basic.TotalEnergyVarioAvailable.clear();
  basic.NettoVarioAvailable.clear();
  basic.ExternalWindAvailable.clear();
  basic.gps.real = false;
  basic.gps.Replay = true;
  basic.gps.Simulator = false;

  TriggerGPSUpdate();
};

/**
 * Stops the replay
 */
void DeviceBlackboard::StopReplay() {
  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();

  basic.GroundSpeed = fixed_zero;
  basic.gps.Replay = false;
}

void
DeviceBlackboard::ProcessSimulation()
{
  if (!is_simulator())
    return;

  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();

  basic.Connected.update(fixed(MonotonicClockMS()) / 1000);
  basic.gps.SatellitesUsed = 6;
  basic.gps.Simulator = true;
  basic.gps.real = false;
  basic.gps.MovementDetected = false;

#ifdef ANDROID
  basic.gps.AndroidInternalGPS = false;
#endif

  basic.Location = FindLatitudeLongitude(basic.Location, basic.TrackBearing,
                                         basic.GroundSpeed);
  basic.LocationAvailable.update(basic.Time);
  basic.GPSAltitudeAvailable.update(basic.Time);
  basic.TrackBearingAvailable.update(basic.Time);
  basic.GroundSpeedAvailable.update(basic.Time);

  basic.Time += fixed_one;
  long tsec = (long)basic.Time;
  basic.DateTime.hour = tsec / 3600;
  basic.DateTime.minute = (tsec - basic.DateTime.hour * 3600) / 60;
  basic.DateTime.second = tsec - basic.DateTime.hour * 3600
    - basic.DateTime.minute * 60;

  // use this to test FLARM parsing/display
  if (is_debug() && !is_altair())
    DeviceList[0].parser.TestRoutine(&basic);

  // clear Airspeed as it is not available in simulation mode
  basic.AirspeedAvailable.clear();

  TriggerGPSUpdate();
}

/**
 * Sets the GPS speed and indicated airspeed to val
 *
 * not in use
 * @param val New speed
 */
void
DeviceBlackboard::SetSpeed(fixed val)
{
  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();

  basic.GroundSpeed = val;
  basic.ProvideBothAirspeeds(val);
}

/**
 * Sets the TrackBearing to val
 *
 * not in use
 * @param val New TrackBearing
 */
void
DeviceBlackboard::SetTrackBearing(Angle val)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().TrackBearing = val.as_bearing();
}

/**
 * Sets the altitude and barometric altitude to val
 *
 * not in use
 * @param val New altitude
 */
void
DeviceBlackboard::SetAltitude(fixed val)
{
  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();

  basic.GPSAltitude = val;
  basic.ProvideBaroAltitudeTrue(NMEA_INFO::BARO_ALTITUDE_UNKNOWN, val);
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
  calculated_info = derived_info;
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
  settings_computer = settings;
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
  settings_map = settings;
}

/**
 * Tries to find a name for every current traffic id
 */
void
DeviceBlackboard::FLARM_ScanTraffic()
{
  // TODO: this is a bit silly, it searches every time a target is
  // visible... going to be slow..
  // should only scan the first time it appears with that ID.
  // at least it is now not being done by the parser

  FLARM_STATE &flarm = SetBasic().flarm;

  // if (FLARM data is available)
  if (!flarm.available)
    return;

  // for each item in traffic
  for (unsigned i = 0; i < flarm.traffic.size(); i++) {
    FLARM_TRAFFIC &traffic = flarm.traffic[i];

    // if (traffic[flarm_slot] has data)
    // and if (Target currently without name)
    if (!traffic.HasName()) {
      // need to lookup name for this target
      const TCHAR *fname = FlarmDetails::LookupCallsign(traffic.ID);
      if (fname != NULL)
        traffic.Name = fname;
    }
  }
}

bool
DeviceBlackboard::expire_wall_clock()
{
  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();
  if (!basic.Connected)
    return false;

  basic.expire_wall_clock();
  return !basic.Connected;
}

void
DeviceBlackboard::tick()
{
  SetBasic().expire();
  calculated_info.expire(Basic().Time);

  // lookup known traffic
  FLARM_ScanTraffic();

  // calculate fast data to complete aircraft state
  NavAltitude();
  AutoQNH();

  if (Basic().Time!= LastBasic().Time) {

    if (Basic().Time > LastBasic().Time) {
      Dynamics();
    }

    state_last = Basic();
  }
}

/**
 * 1. Determines which altitude to use (GPS/baro)
 * 2. Calculates height over ground
 */
void
DeviceBlackboard::NavAltitude()
{
  NMEA_INFO &basic = SetBasic();

  if (!SettingsComputer().EnableNavBaroAltitude
      || !basic.BaroAltitudeAvailable) {
    basic.NavAltitude = basic.GPSAltitude;
  } else {
    basic.NavAltitude = basic.BaroAltitude;
  }
}

/**
 * Calculates the turn rate of the heading,
 * the estimated bank angle and
 * the estimated pitch angle
 */
void
DeviceBlackboard::Dynamics()
{
  NMEA_INFO &basic = SetBasic();

  if (Calculated().flight.Flying &&
      (positive(basic.GroundSpeed) || Calculated().wind.is_non_zero())) {

    // estimate bank angle (assuming balanced turn)
    if (Calculated().AirspeedAvailable) {
      const fixed angle = atan(Angle::degrees(Calculated().TurnRateWind
          * Calculated().TrueAirspeed * fixed_inv_g).value_radians());

      basic.acceleration.BankAngle = Angle::radians(angle);
      if (!basic.acceleration.Available)
        basic.acceleration.Gload = fixed_one / max(fixed_small, fabs(cos(angle)));
    } else {
      basic.acceleration.BankAngle = Angle::native(fixed_zero);
      if (!basic.acceleration.Available)
        basic.acceleration.Gload = fixed_one;
    }

    // estimate pitch angle (assuming balanced turn)
    if (Calculated().AirspeedAvailable && basic.TotalEnergyVarioAvailable)
      basic.acceleration.PitchAngle = Angle::radians(atan2(Calculated().GPSVario - basic.TotalEnergyVario,
          Calculated().TrueAirspeed));
    else
      basic.acceleration.PitchAngle = Angle::native(fixed_zero);

  } else {
    basic.acceleration.BankAngle = Angle::native(fixed_zero);
    basic.acceleration.PitchAngle = Angle::native(fixed_zero);

    if (!basic.acceleration.Available)
      basic.acceleration.Gload = fixed_one;
  }
}

void
DeviceBlackboard::AutoQNH()
{
  NMEA_INFO &basic = SetBasic();

  #define QNH_TIME 10

  static unsigned countdown_autoqnh = QNH_TIME;

  if (!Calculated().flight.OnGround // must be on ground
      || !countdown_autoqnh    // only do it once
      || !basic.gps.real // never in replay mode / simulator
      || !basic.LocationAvailable // Reject if no valid GPS fix
      || !basic.PressureAltitudeAvailable // Reject if no pressure altitude
      || basic.QNHAvailable // Reject if QNH already known
    ) {
    if (countdown_autoqnh<= QNH_TIME) {
      countdown_autoqnh= QNH_TIME; // restart if havent performed
    }
    return;
  }

  if (countdown_autoqnh<= QNH_TIME)
    countdown_autoqnh--;

  if (!countdown_autoqnh) {
    const Waypoint *next_wp;
    next_wp = way_points.lookup_location(basic.Location, fixed(1000));

    if (next_wp && next_wp->is_airport()) {
      basic.ProvideQNHSetting(basic.pressure.FindQNHFromPressureAltitude(basic.PressureAltitude, next_wp->Altitude));
    } else if (Calculated().TerrainValid) {
      basic.ProvideQNHSetting(basic.pressure.FindQNHFromPressureAltitude(basic.PressureAltitude, Calculated().TerrainAlt));
    } else
      return;

    AllDevicesPutQNH(basic.pressure);
    countdown_autoqnh = UINT_MAX; // disable after performing once
  }
}

void
DeviceBlackboard::SetQNH(fixed qnh)
{
  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();

  basic.ProvideQNHSetting(qnh);
  AllDevicesPutQNH(basic.pressure);
}

void
DeviceBlackboard::SetMC(fixed mc)
{
  ScopeLock protect(mutexBlackboard);
  NMEA_INFO &basic = SetBasic();

  basic.MacCready = mc;
  AllDevicesPutMacCready(mc);
}
