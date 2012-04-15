/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Task/TaskManager.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"

#define fixed_300 fixed(300)

bool
DemoReplayGlue::UpdateTime()
{
  if (!clock.Check(1000))
    return false;
  clock.Update();
  return true;
}

void
DemoReplayGlue::ResetTime()
{
  clock.Reset();
}

void
DemoReplayGlue::OnAdvance(const GeoPoint &loc, const fixed speed,
                           const Angle bearing, const fixed alt,
                           const fixed baroalt, const fixed t)
{
  device_blackboard->SetLocation(loc, speed, bearing, alt, baroalt, t);
}

void
DemoReplayGlue::Start()
{
  ProtectedTaskManager::ExclusiveLease protected_task_manager(*task_manager);
  const TaskAccessor ta(protected_task_manager, fixed_zero);
  parms.realistic();
  parms.start_alt = device_blackboard->Basic().nav_altitude;
  DemoReplay::Start(ta, device_blackboard->Basic().location);

  // get wind from aircraft
  aircraft.GetState().wind = device_blackboard->Calculated().GetWindOrZero();
}

void
DemoReplayGlue::OnStop()
{
  device_blackboard->StopReplay();
}

bool
DemoReplayGlue::Update()
{
  if (!enabled)
    return false;

  if (!UpdateTime())
    return true;

  fixed floor_alt = fixed_300;
  if (device_blackboard->Calculated().terrain_valid) {
    floor_alt += device_blackboard->Calculated().terrain_altitude;
  }

  ProtectedTaskManager::ExclusiveLease protected_task_manager(*task_manager);
  TaskAccessor ta(protected_task_manager, floor_alt);
  bool retval = DemoReplay::Update(ta);

  const AircraftState s = aircraft.GetState();
  OnAdvance(s.location, s.ground_speed, s.track, s.altitude,
             s.altitude, s.time);

  return retval;
}
