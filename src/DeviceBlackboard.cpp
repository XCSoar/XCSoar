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
#include "FLARM/FlarmCalculations.h"
#include "Asset.hpp"
#include "Device/All.hpp"
#include "Math/Constants.h"
#include "GlideSolvers/GlidePolar.hpp"
#include "Simulator.hpp"
#include "OS/Clock.hpp"

#include <limits.h>

#ifdef WIN32
#include <windows.h>
#endif

static FlarmCalculations flarmCalculations;

DeviceBlackboard device_blackboard;

/**
 * Initializes the DeviceBlackboard
 */
void
DeviceBlackboard::Initialise()
{
  ScopeLock protect(mutexBlackboard);

  last_location_available.clear();
  last_te_vario_available.clear();
  last_netto_vario_available.clear();

  // Clear the gps_info and calculated_info
  gps_info.reset();
  calculated_info.reset();

  // Set GPS assumed time to system time
  gps_info.DateTime = BrokenDateTime::NowUTC();
  gps_info.Time = fixed(gps_info.DateTime.hour * 3600 +
                        gps_info.DateTime.minute * 60 +
                        gps_info.DateTime.second);

  for (unsigned i = 0; i < NUMDEV; ++i)
    per_device_data[i] = gps_info;

  real_data = simulator_data = replay_data = gps_info;
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

  if (Calculated().flight.Flying)
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    if (!per_device_data[i].LocationAvailable)
      per_device_data[i].SetFakeLocation(loc, alt);

  if (!real_data.LocationAvailable)
    real_data.SetFakeLocation(loc, alt);

  simulator_data.SetFakeLocation(loc, alt);
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
  NMEA_INFO &basic = SetReplayState();

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
  basic.ProvidePressureAltitude(baroalt);
  basic.ProvideBaroAltitudeTrue(baroalt);
  basic.Time = t;
  basic.TotalEnergyVarioAvailable.clear();
  basic.NettoVarioAvailable.clear();
  basic.ExternalWindAvailable.clear();
  basic.gps.real = false;
  basic.gps.Replay = true;
  basic.gps.Simulator = false;

  Merge();
};

/**
 * Stops the replay
 */
void DeviceBlackboard::StopReplay() {
  ScopeLock protect(mutexBlackboard);

  replay_data.Connected.clear();

  Merge();
}

void
DeviceBlackboard::ProcessSimulation()
{
  if (!is_simulator())
    return;

  ScopeLock protect(mutexBlackboard);

  simulator.Process(simulator_data);
  Merge();
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
  NMEA_INFO &basic = simulator_data;

  basic.GroundSpeed = val;
  basic.ProvideBothAirspeeds(val);

  Merge();
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
  simulator_data.TrackBearing = val.as_bearing();

  Merge();
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
  NMEA_INFO &basic = simulator_data;

  basic.GPSAltitude = val;
  basic.ProvidePressureAltitude(val);
  basic.ProvideBaroAltitudeTrue(val);

  Merge();
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
 * Tries to find a name for every current traffic id
 */
void
DeviceBlackboard::ProcessFLARM()
{
  // TODO: this is a bit silly, it searches every time a target is
  // visible... going to be slow..
  // should only scan the first time it appears with that ID.
  // at least it is now not being done by the parser

  const NMEA_INFO &basic = Basic();
  FLARM_STATE &flarm = SetBasic().flarm;

  // if (FLARM data is available)
  if (!flarm.available)
    return;

  // calculate relative east and north projection to lat/lon
  Angle delta_lat = Angle::degrees(fixed(0.01));
  Angle delta_lon = Angle::degrees(fixed(0.01));

  GeoPoint plat = basic.Location;
  plat.Latitude += delta_lat;
  GeoPoint plon = basic.Location;
  plon.Longitude += delta_lon;

  fixed dlat = Distance(basic.Location, plat);
  fixed dlon = Distance(basic.Location, plon);

  fixed FLARM_NorthingToLatitude(0);
  fixed FLARM_EastingToLongitude(0);

  if (positive(fabs(dlat)) && positive(fabs(dlon))) {
    FLARM_NorthingToLatitude = delta_lat.value_degrees() / dlat;
    FLARM_EastingToLongitude = delta_lon.value_degrees() / dlon;
  }

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

    // 1 relativenorth, meters
    traffic.Location.Latitude =
        Angle::degrees(traffic.RelativeNorth * FLARM_NorthingToLatitude) +
        basic.Location.Latitude;

    // 2 relativeeast, meters
    traffic.Location.Longitude =
        Angle::degrees(traffic.RelativeEast * FLARM_EastingToLongitude) +
        basic.Location.Longitude;

    // alt
    traffic.Altitude = traffic.RelativeAltitude + basic.GPSAltitude;

    traffic.Average30s =
        flarmCalculations.Average30s(traffic.ID, basic.Time, traffic.Altitude);
  }
}

bool
DeviceBlackboard::expire_wall_clock()
{
  ScopeLock protect(mutexBlackboard);
  if (!Basic().Connected)
    return false;

  for (unsigned i = 0; i < NUMDEV; ++i)
    per_device_data[i].expire_wall_clock();

  Merge();
  return !Basic().Connected;
}

void
DeviceBlackboard::Merge()
{
  real_data.reset();
  for (unsigned i = 0; i < NUMDEV; ++i) {
    per_device_data[i].expire();
    real_data.complement(per_device_data[i]);
  }

  if (replay_data.Connected) {
    replay_data.expire();
    SetBasic() = replay_data;
  } else if (simulator_data.Connected) {
    simulator_data.expire();
    SetBasic() = simulator_data;
  } else {
    SetBasic() = real_data;
  }

  computer.Fill(SetBasic(), SettingsComputer());

  if (last_location_available != Basic().LocationAvailable) {
    last_location_available = Basic().LocationAvailable;
    TriggerGPSUpdate();
  }

  if (last_te_vario_available != Basic().TotalEnergyVarioAvailable ||
      last_netto_vario_available != Basic().NettoVarioAvailable) {
    last_te_vario_available = Basic().TotalEnergyVarioAvailable;
    last_netto_vario_available = Basic().NettoVarioAvailable;
    TriggerVarioUpdate();
  }
}

void
DeviceBlackboard::tick()
{
  SetBasic().expire();

  // lookup known traffic
  ProcessFLARM();

  // calculate fast data to complete aircraft state

  computer.Compute(SetBasic(), LastBasic(),
                   Calculated(), SettingsComputer());

  if (Basic().Time!= LastBasic().Time) {
    state_last = Basic();
  }
}

void
DeviceBlackboard::SetQNH(fixed qnh)
{
  ScopeLock protect(mutexBlackboard);

  AllDevicesPutQNH(AtmosphericPressure(qnh), Calculated());
}

void
DeviceBlackboard::SetMC(fixed mc)
{
  AllDevicesPutMacCready(mc);
}
