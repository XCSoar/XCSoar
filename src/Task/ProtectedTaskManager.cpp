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

#include "ProtectedTaskManager.hpp"
#include "Task/TaskManager.hpp"
#include "Util/Serialiser.hpp"
#include "Util/Deserialiser.hpp"
#include "Util/DataNodeXML.hpp"
#include "Task/TaskFile.hpp"
#include "LocalPath.hpp"
#include "Terrain/RasterTerrain.hpp"

#include <windef.h> // for MAX_PATH

ProtectedTaskManager::~ProtectedTaskManager() {
  ExclusiveLease lease(*this);
  lease->set_intersection_test(NULL); // de-register
}

void 
ProtectedTaskManager::set_glide_polar(const GlidePolar& glide_polar)
{
  ExclusiveLease lease(*this);
  lease->set_glide_polar(glide_polar);
}

fixed
ProtectedTaskManager::GetMacCready() const
{
  Lease lease(*this);
  return lease->get_glide_polar().GetMC();
}

TaskManager::TaskMode_t 
ProtectedTaskManager::get_mode() const
{
  Lease lease(*this);
  return lease->get_mode();
}

const OrderedTaskBehaviour 
ProtectedTaskManager::get_ordered_task_behaviour() const
{
  Lease lease(*this);
  return lease->get_ordered_task_behaviour();
}

const Waypoint* 
ProtectedTaskManager::getActiveWaypoint() const
{
  Lease lease(*this);
  const TaskWaypoint *tp = lease->getActiveTaskPoint();
  if (tp)
    return &tp->get_waypoint();

  return NULL;
}

bool
ProtectedTaskManager::isInSector (const unsigned TPindex, const AIRCRAFT_STATE &ref) const
{
  Lease lease(*this);
  return lease->isInSector(TPindex, ref);
}

const GeoPoint&
ProtectedTaskManager::get_location_target(const unsigned TPindex, const GeoPoint& fallback_location) const
{
  Lease lease(*this);
  return lease->get_location_target(TPindex, fallback_location);
}

bool
ProtectedTaskManager::target_is_locked(const unsigned TPindex) const
{
  Lease lease(*this);
  return lease->target_is_locked(TPindex);
}

bool
ProtectedTaskManager::has_target(const unsigned TPindex) const
{
  Lease lease(*this);
  return lease->has_target(TPindex);
}

bool
ProtectedTaskManager::set_target(const unsigned TPindex, const GeoPoint &loc,
   const bool override_lock)
{
  ExclusiveLease lease(*this);
  return lease->set_target(TPindex, loc, override_lock);
}

bool
ProtectedTaskManager::set_target(const unsigned TPindex, const fixed range,
   const fixed radial)
{
  ExclusiveLease lease(*this);
  return lease->set_target(TPindex, range, radial);
}

bool
ProtectedTaskManager::get_target_range_radial(const unsigned TPindex, fixed &range,
                                              fixed &radial) const
{
  Lease lease(*this);
  return lease->get_target_range_radial(TPindex, range, radial);
}

bool
ProtectedTaskManager::target_lock(const unsigned TPindex, bool do_lock)
{
  ExclusiveLease lease(*this);
  return lease->target_lock(TPindex, do_lock);
}

const GeoPoint&
ProtectedTaskManager::get_ordered_taskpoint_location(const unsigned TPindex,
   const GeoPoint& fallback_location) const
{
  Lease lease(*this);
  return lease->get_ordered_taskpoint_location(TPindex, fallback_location);
}

fixed
ProtectedTaskManager::get_ordered_taskpoint_radius(const unsigned TPindex) const
{
  Lease lease(*this);
  return lease->get_ordered_taskpoint_radius(TPindex);
}

const TCHAR*
ProtectedTaskManager::get_ordered_taskpoint_name(const unsigned TPindex) const
{
 Lease lease(*this);
 return lease->get_ordered_taskpoint_name(TPindex);
}

void 
ProtectedTaskManager::incrementActiveTaskPoint(int offset)
{
  ExclusiveLease lease(*this);
  lease->incrementActiveTaskPoint(offset);
}

void 
ProtectedTaskManager::incrementActiveTaskPointArm(int offset)
{
  ExclusiveLease lease(*this);

  switch (lease->get_task_advance().get_advance_state()) {
  case TaskAdvance::MANUAL:
  case TaskAdvance::AUTO:
    lease->incrementActiveTaskPoint(offset);
    break;
  case TaskAdvance::START_DISARMED:
  case TaskAdvance::TURN_DISARMED:
    if (offset>0) {
      lease->get_task_advance().set_armed(true);
    } else {
      lease->incrementActiveTaskPoint(offset);
    }
    break;
  case TaskAdvance::START_ARMED:
  case TaskAdvance::TURN_ARMED:
    if (offset>0) {
      lease->incrementActiveTaskPoint(offset);
    } else {
      lease->get_task_advance().set_armed(false);
    }
    break;
  default:
    assert(1);
  }
}

bool 
ProtectedTaskManager::do_goto(const Waypoint & wp)
{
  ExclusiveLease lease(*this);
  return lease->do_goto(wp);
}

AIRCRAFT_STATE 
ProtectedTaskManager::get_start_state() const
{
  Lease lease(*this);
  return lease->get_start_state();
}

fixed 
ProtectedTaskManager::get_finish_height() const
{
  Lease lease(*this);
  return lease->get_finish_height();
}

OrderedTask*
ProtectedTaskManager::task_clone() const
{
  Lease lease(*this);
  return lease->clone(task_events, task_behaviour,
                      lease->get_glide_polar());
}

OrderedTask* 
ProtectedTaskManager::task_copy(const OrderedTask& that) const
{
  Lease lease(*this);
  return that.clone(task_events, task_behaviour, lease->get_glide_polar());
}

OrderedTask* 
ProtectedTaskManager::task_blank() const
{
  Lease lease(*this);
  return new OrderedTask(task_events, task_behaviour, lease->get_glide_polar());
}

bool
ProtectedTaskManager::task_commit(const OrderedTask& that)
{
  ExclusiveLease lease(*this);
  return lease->commit(that);
}

bool 
ProtectedTaskManager::task_save(const TCHAR* path, const OrderedTask& task)
{
  DataNodeXML* root = DataNodeXML::createRoot(_T("Task"));
  Serialiser tser(*root);
  tser.serialise(task);

  bool retval = root->save(path);
  delete root;  
  return retval;
}

bool 
ProtectedTaskManager::task_save(const TCHAR* path)
{
  OrderedTask* task = task_clone();
  bool retval = task_save(path, *task);
  delete task;
  return retval;
}

OrderedTask* 
ProtectedTaskManager::task_create(const TCHAR* path, const Waypoints *waypoints,
                                  unsigned index) const
{
  TaskFile* task_file = TaskFile::Create(path);
  if (task_file != NULL) {
    OrderedTask* task = task_file->GetTask(waypoints, index);
    delete task_file;
    return task;
  }
  return NULL;
}
 
bool 
ProtectedTaskManager::task_load(const TCHAR* path, const Waypoints *waypoints,
                                unsigned index)
{
  OrderedTask* task = task_create(path, waypoints, index);
  if (task != NULL) {
    ExclusiveLease lease(*this);
    lease->commit(*task);
    lease->resume();
    delete task;
    return true;
  }
  return false;
}

const TCHAR ProtectedTaskManager::default_task_path[] = _T("Default.tsk");

bool 
ProtectedTaskManager::task_load_default(const Waypoints *waypoints)
{
  TCHAR path[MAX_PATH];
  LocalPath(path, default_task_path);
  return task_load(path, waypoints);
}

bool 
ProtectedTaskManager::task_save_default()
{
  TCHAR path[MAX_PATH];
  LocalPath(path, default_task_path);
  return task_save(path);
}

void 
ProtectedTaskManager::reset()
{
  ExclusiveLease lease(*this);
  lease->reset();
}

void
ProtectedTaskManager::route_set_terrain(const RasterTerrain *terrain)
{
  ExclusiveLease lease(*this);
  m_route.set_terrain(terrain);
  intersection_test.set_route(&m_route);
  lease->set_intersection_test(&intersection_test);
}

void
ProtectedTaskManager::route_solve(const AGeoPoint& dest, const AGeoPoint& start,
                                  const short h_ceiling)
{
  ExclusiveLease lease(*this);
  m_route.synchronise(m_airspaces, dest, start);
  m_route.solve(dest, start, lease->get_task_behaviour().route_planner, h_ceiling);
  m_route.get_solution(lease->get_planned_route());
}

void
ProtectedTaskManager::route_update_polar(const SpeedVector& wind)
{
  ExclusiveLease lease(*this);
  m_route.update_polar(lease->get_glide_polar(),
                       lease->get_safety_polar(),
                       wind);
}

GlidePolar
ProtectedTaskManager::get_reach_polar() const
{
  Lease lease(*this);
  return m_route.get_reach_polar();
}

bool
ProtectedTaskManager::intersection(const AGeoPoint& origin,
                                   const AGeoPoint& destination,
                                   GeoPoint& intx) const
{
  return m_route.intersection(origin, destination, intx);
}

bool
ReachIntersectionTest::intersects(const AGeoPoint& destination)
{
  if (!route)
    return false;
  short h, h_dummy;
  route->find_positive_arrival(destination, h, h_dummy);
  // we use find_positive_arrival here instead of is_inside, because may use
  // arrival height for sorting later
  return (h< destination.altitude);
}

void
ProtectedTaskManager::solve_reach(const AGeoPoint& origin, const bool do_solve)
{
  ExclusiveLease lease(*this);
  m_route.solve_reach(origin, do_solve);
}

bool
ProtectedTaskManager::find_positive_arrival(const AGeoPoint& dest,
                                            short& arrival_height_reach,
                                            short& arrival_height_direct) const
{
  Lease lease(*this);
  return m_route.find_positive_arrival(dest, arrival_height_reach, arrival_height_direct);
}

void
ProtectedTaskManager::accept_in_range(const GeoBounds& bounds,
                                      TriangleFanVisitor& visitor) const
{
  Lease lease(*this);
  return m_route.accept_in_range(bounds, visitor);
}

short
ProtectedTaskManager::get_terrain_base() const
{
  Lease lease(*this);
  return m_route.get_terrain_base();
}
