// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Replay/DemoReplayGlue.hpp"
#include "TaskAccessor.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "NMEA/Info.hpp"

DemoReplayGlue::DemoReplayGlue(DeviceBlackboard &_device_blackboard,
                               ProtectedTaskManager &_task_manager) noexcept
  :device_blackboard(_device_blackboard),
   task_manager(_task_manager)
{
  ProtectedTaskManager::ExclusiveLease protected_task_manager{task_manager};
  const TaskAccessor ta(protected_task_manager, 0);
  parms.SetRealistic();
  parms.start_alt = device_blackboard.Basic().nav_altitude;
  DemoReplay::Start(ta, device_blackboard.Basic().location);

  // get wind from aircraft
  aircraft.GetState().wind = device_blackboard.Calculated().GetWindOrZero();
}

bool
DemoReplayGlue::Update(NMEAInfo &data)
{
  double floor_alt = 300;
  if (device_blackboard.Calculated().terrain_valid) {
    floor_alt += device_blackboard.Calculated().terrain_altitude;
  }

  bool retval;

  {
    ProtectedTaskManager::ExclusiveLease protected_task_manager{task_manager};
    TaskAccessor ta(protected_task_manager, floor_alt);
    retval = DemoReplay::Update(std::chrono::seconds{1}, ta);
  }

  const AircraftState &s = aircraft.GetState();

  data.clock = s.time;
  data.alive.Update(data.clock);
  data.ProvideTime(s.time);
  data.location = s.location;
  data.location_available.Update(data.clock);
  data.ground_speed = s.ground_speed;
  data.ground_speed_available.Update(data.clock);
  data.track = s.track;
  data.track_available.Update(data.clock);
  data.gps_altitude = s.altitude;
  data.gps_altitude_available.Update(data.clock);
  data.ProvidePressureAltitude(s.altitude);
  data.ProvideBaroAltitudeTrue(s.altitude);
  data.gps.real = false;
  data.gps.replay = true;
  data.gps.simulator = false;

  return retval;
}
