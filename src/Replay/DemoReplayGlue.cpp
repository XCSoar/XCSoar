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

#include "Replay/DemoReplayGlue.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "DeviceBlackboard.hpp"
#include "Task/TaskManager.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"

#define fixed_300 fixed(300)

class ProtectedTaskAccessor: public TaskAccessor {
public:
  ProtectedTaskAccessor(ProtectedTaskManager::ExclusiveLease& _task,
    const fixed _floor_alt):
    task(_task), floor_alt(_floor_alt) {};

  bool is_ordered() const {
    return task->task_size()>1;
  }
  bool is_empty() const {
    return task->task_size()==0;
  }
  bool is_finished() const {
    return task->get_common_stats().task_finished;
  }
  bool is_started() const {
    return task->get_common_stats().task_started;
  }
  GeoPoint random_oz_point(unsigned index, const fixed noise) const {
    return task->random_point_in_task(index, noise);
  }
  unsigned size() const {
    return task->task_size();
  }
  GeoPoint getActiveTaskPointLocation() const {
    return task->getActiveTaskPoint()->GetLocation();
  }
  bool has_entered(unsigned index) const {
    AbstractTaskFactory &fact = task->get_factory();
    return fact.has_entered(index);
  }
  const ElementStat leg_stats() const {
    return task->get_stats().current_leg;
  }
  fixed target_height() const {
    if (task->getActiveTaskPoint()) {
      return max(floor_alt, task->getActiveTaskPoint()->GetElevation());
    } else {
      return floor_alt;
    }
  }
  fixed distance_to_final() const {
    return task->get_stats().total.solution_remaining.distance_to_final;
  }
  fixed remaining_alt_difference() const {
    return task->get_stats().total.solution_remaining.altitude_difference;
  }
  GlidePolar get_glide_polar() const {
    return task->get_glide_polar();
  }
  void setActiveTaskPoint(unsigned index) {
    task->setActiveTaskPoint(index);
  }
  unsigned getActiveTaskPointIndex() const {
    return task->getActiveTaskPointIndex();
  }
private:
  ProtectedTaskManager::ExclusiveLease& task;
  const fixed floor_alt;
};


bool
DemoReplayGlue::UpdateTime()
{
  if (!clock.check(1000))
    return false;
  clock.update();
  return true;
}

void
DemoReplayGlue::ResetTime()
{
  clock.reset();
}

void
DemoReplayGlue::on_advance(const GeoPoint &loc, const fixed speed,
                           const Angle bearing, const fixed alt,
                           const fixed baroalt, const fixed t)
{
  device_blackboard.SetLocation(loc, speed, bearing, alt, baroalt, t);
}

void
DemoReplayGlue::Start()
{
  ProtectedTaskManager::ExclusiveLease task_manager(*m_task_manager);
  ProtectedTaskAccessor ta(task_manager, fixed_zero);
  parms.realistic();
  parms.start_alt = device_blackboard.Basic().nav_altitude;
  DemoReplay::Start(ta, device_blackboard.Basic().location);

  // get wind from aircraft
  aircraft.GetState().wind = device_blackboard.Calculated().wind;
}

void
DemoReplayGlue::on_stop()
{
  device_blackboard.StopReplay();
}

bool
DemoReplayGlue::Update()
{
  if (!enabled)
    return false;

  if (!UpdateTime())
    return true;

  fixed floor_alt = fixed_300;
  if (device_blackboard.Calculated().terrain_valid) {
    floor_alt += device_blackboard.Calculated().terrain_altitude;
  }

  ProtectedTaskManager::ExclusiveLease task_manager(*m_task_manager);
  ProtectedTaskAccessor ta(task_manager, floor_alt);
  bool retval = DemoReplay::Update(ta);

  const AircraftState s = aircraft.GetState();
  on_advance(s.location, s.ground_speed, s.track, s.altitude,
             s.altitude, s.time);

  return retval;
}
