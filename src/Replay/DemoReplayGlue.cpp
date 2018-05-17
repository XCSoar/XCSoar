/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Replay/DemoReplayGlue.hpp"
#include "TaskAccessor.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "NMEA/Info.hpp"

DemoReplayGlue::DemoReplayGlue(ProtectedTaskManager &_task_manager)
  :task_manager(&_task_manager)
{
  ProtectedTaskManager::ExclusiveLease protected_task_manager(*task_manager);
  const TaskAccessor ta(protected_task_manager, 0);
  parms.SetRealistic();
  parms.start_alt = device_blackboard->Basic().nav_altitude;
  DemoReplay::Start(ta, device_blackboard->Basic().location);

  // get wind from aircraft
  aircraft.GetState().wind = device_blackboard->Calculated().GetWindOrZero();
}

bool
DemoReplayGlue::Update(NMEAInfo &data)
{
  double floor_alt = 300;
  if (device_blackboard->Calculated().terrain_valid) {
    floor_alt += device_blackboard->Calculated().terrain_altitude;
  }

  bool retval;

  {
    ProtectedTaskManager::ExclusiveLease protected_task_manager(*task_manager);
    TaskAccessor ta(protected_task_manager, floor_alt);
    retval = DemoReplay::Update(1, ta);
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
