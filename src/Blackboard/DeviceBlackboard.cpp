/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Blackboard/DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "Device/All.hpp"
#include "Simulator.hpp"

#include <algorithm>

/**
 * Initializes the DeviceBlackboard
 */
DeviceBlackboard::DeviceBlackboard()
{
  // Clear the gps_info and calculated_info
  gps_info.Reset();
  calculated_info.Reset();

  // Set GPS assumed time to system time
  gps_info.UpdateClock();
  gps_info.date_time_utc = BrokenDateTime::NowUTC();
  gps_info.time = fixed(gps_info.date_time_utc.GetSecondOfDay());

  std::fill_n(per_device_data, NUMDEV, gps_info);

  real_data = simulator_data = replay_data = gps_info;

  simulator.Init(simulator_data);

  real_clock.Reset();
  replay_clock.Reset();
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
  ScopeLock protect(mutex);

  if (Calculated().flight.flying)
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    if (!per_device_data[i].location_available)
      per_device_data[i].SetFakeLocation(loc, alt);

  if (!real_data.location_available)
    real_data.SetFakeLocation(loc, alt);

  if (is_simulator()) {
    simulator_data.SetFakeLocation(loc, alt);
    simulator.Touch(simulator_data);
  }

  ScheduleMerge();
}

/**
 * Stops the replay
 */
void DeviceBlackboard::StopReplay() {
  ScopeLock protect(mutex);

  replay_data.Reset();

  ScheduleMerge();
}

void
DeviceBlackboard::ProcessSimulation()
{
  if (!is_simulator())
    return;

  ScopeLock protect(mutex);

  simulator.Process(simulator_data);
  ScheduleMerge();
}

void
DeviceBlackboard::SetSimulatorLocation(const GeoPoint &location)
{
  ScopeLock protect(mutex);
  NMEAInfo &basic = simulator_data;

  simulator.Touch(basic);
  basic.track = location.Bearing(basic.location).Reciprocal();
  basic.location = location;

  ScheduleMerge();
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
  ScopeLock protect(mutex);
  NMEAInfo &basic = simulator_data;

  simulator.Touch(basic);
  basic.ground_speed = val;

  ScheduleMerge();
}

/**
 * Sets the TrackBearing to val
 *
 * not in use
 * @param val New TrackBearing
 */
void
DeviceBlackboard::SetTrack(Angle val)
{
  ScopeLock protect(mutex);
  simulator.Touch(simulator_data);
  simulator_data.track = val.AsBearing();

  ScheduleMerge();
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
  ScopeLock protect(mutex);
  NMEAInfo &basic = simulator_data;

  simulator.Touch(basic);
  basic.gps_altitude = val;

  ScheduleMerge();
}

/**
 * Reads the given derived_info usually provided by the
 * GlideComputerBlackboard and saves it to the own Blackboard
 * @param derived_info Calculated information usually provided
 * by the GlideComputerBlackboard
 */
void
DeviceBlackboard::ReadBlackboard(const DerivedInfo &derived_info)
{
  calculated_info = derived_info;
}

/**
 * Reads the given settings usually provided by the InterfaceBlackboard
 * and saves it to the own Blackboard
 * @param settings ComputerSettings usually provided by the
 * InterfaceBlackboard
 */
void
DeviceBlackboard::ReadComputerSettings(const ComputerSettings &settings)
{
  computer_settings = settings;
}

void
DeviceBlackboard::ExpireWallClock()
{
  ScopeLock protect(mutex);
  if (!Basic().alive)
    return;

  bool modified = false;
  for (unsigned i = 0; i < NUMDEV; ++i) {
    NMEAInfo &basic = per_device_data[i];
    if (!basic.alive)
      continue;

    basic.ExpireWallClock();
    if (!basic.alive)
      modified = true;
  }

  if (modified)
    ScheduleMerge();
}

void
DeviceBlackboard::ScheduleMerge()
{
  TriggerMergeThread();
}

void
DeviceBlackboard::Merge()
{
  real_data.Reset();
  for (unsigned i = 0; i < NUMDEV; ++i) {
    if (!per_device_data[i].alive)
      continue;

    per_device_data[i].UpdateClock();
    per_device_data[i].Expire();
    real_data.Complement(per_device_data[i]);
  }

  real_clock.Normalise(real_data);
  replay_clock.Normalise(replay_data);

  if (replay_data.alive) {
    /* the replay may run at a higher speed; use NMEA_INFO::Time as a
       "fake wallclock" to prevent them from expiring too quickly */
    replay_data.clock = replay_data.time;

    replay_data.Expire();
    SetBasic() = replay_data;
  } else if (simulator_data.alive) {
    simulator_data.UpdateClock();
    simulator_data.Expire();
    SetBasic() = simulator_data;
  } else {
    SetBasic() = real_data;
  }
}

void
DeviceBlackboard::SetBallast(fixed fraction, fixed overload,
                             OperationEnvironment &env)
{
  AllDevicesPutBallast(fraction, overload, env);
}

void
DeviceBlackboard::SetBugs(fixed bugs, OperationEnvironment &env)
{
  AllDevicesPutBugs(bugs, env);
}

void
DeviceBlackboard::SetQNH(AtmosphericPressure qnh, OperationEnvironment &env)
{
  AllDevicesPutQNH(qnh, env);
}

void
DeviceBlackboard::SetMC(fixed mc, OperationEnvironment &env)
{
  AllDevicesPutMacCready(mc, env);
}
