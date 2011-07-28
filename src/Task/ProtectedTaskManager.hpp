/* Copyright_License {

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

#ifndef XCSOAR_PROTECTED_TASK_MANAGER_HPP
#define XCSOAR_PROTECTED_TASK_MANAGER_HPP

#include "Thread/Guard.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskManager.hpp"
#include "Task/TaskAdvance.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/RoutePlannerGlue.hpp"
#include "Compiler.h"

class TaskStats;
class CommonStats;
class RasterTerrain;
class Airspaces;

class ReachIntersectionTest: public AbortIntersectionTest {
public:
  ReachIntersectionTest(): route(NULL) {};
  void set_route(RoutePlannerGlue* _route) {
    route = _route;
  }
  virtual bool intersects(const AGeoPoint& destination);
private:
  const RoutePlannerGlue *route;
};

/**
 * Facade to task/airspace/waypoints as used by threads,
 * to manage locking
 */
class ProtectedTaskManager: public Guard<TaskManager>
{
protected:
  const TaskBehaviour &task_behaviour;
  TaskEvents &task_events;
  Airspaces &m_airspaces;
  RoutePlannerGlue m_route;
  ReachIntersectionTest intersection_test;

  static const TCHAR default_task_path[];

public:
  ProtectedTaskManager(TaskManager &_task_manager, const TaskBehaviour& tb,
                       TaskEvents& te,
                       Airspaces &airspaces)
    :Guard<TaskManager>(_task_manager),
     task_behaviour(tb), task_events(te), m_airspaces(airspaces),
     m_route(_task_manager.get_glide_polar(), m_airspaces)
    {}
  
  ~ProtectedTaskManager();

  // common accessors for ui and calc clients
  void set_glide_polar(const GlidePolar& glide_polar);

  gcc_pure
  fixed GetMacCready() const;

  gcc_pure
  TaskManager::TaskMode_t get_mode() const;

  gcc_pure
  const OrderedTaskBehaviour get_ordered_task_behaviour() const;

  gcc_pure
  const Waypoint* getActiveWaypoint() const;

  void incrementActiveTaskPoint(int offset);
  void incrementActiveTaskPointArm(int offset);

  bool do_goto(const Waypoint & wp);

  gcc_pure
  AircraftState get_start_state() const;

  gcc_pure
  fixed get_finish_height() const;

  gcc_malloc
  OrderedTask* task_clone() const;

  gcc_malloc
  OrderedTask* task_blank() const;

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @return True if this task changed
   */
  bool task_commit(const OrderedTask& that);

  bool task_save(const TCHAR* path);
  bool task_load(const TCHAR* path, const Waypoints *waypoints,
                 unsigned index = 0);
  bool task_save_default();
  bool task_load_default(const Waypoints *waypoints);

  gcc_malloc
  OrderedTask* task_copy(const OrderedTask& that) const;

  gcc_malloc
  OrderedTask *task_create(const TCHAR *path, const Waypoints *waypoints,
                           unsigned index = 0) const;
  bool task_save(const TCHAR* path, const OrderedTask& task);

  /** Reset the tasks (as if never flown) */
  void reset();

  /**
   * Check whether observer is within OZ of specified tp
   *
   * @param TPindex index of tp in task
   * @param ref state of aircraft to be checked
   *
   * @return True if reference point is inside sector
   */
  gcc_pure
  bool isInSector (const unsigned TPindex, const AircraftState &ref) const;

  /**
   * Accessor to get target location of specified tp
   *
   * @param TPindex index of tp in task
   *
   * @return Target location or fallback_location if TPindex is
   *    invalid or has no target
   */
  gcc_pure
  const GeoPoint& get_location_target(const unsigned TPindex,
     const GeoPoint& fallback_location) const;

  /**
   * Accessor for locked state of target of specified tp
   *
   * @param TPindex index of tp in task
   *
   * @return True if target is locked or tp location if has no target
   */
  gcc_pure
  bool target_is_locked(const unsigned TPindex) const;

  /**
   * Capability of specified TaskPoint to have adjustable range (true for AAT)
   *
   * @param TPindex index of tp in task
   *
   * @return True if task point has a target (can have range set)
   */
  gcc_pure
  bool has_target(const unsigned TPindex) const;

  /**
   * Set target location explicitly of specified tp
   *
   * @param TPindex index of tp in task
   * @param loc Location of new target
   * @param override_lock If false, won't set the target if it is locked
   */
  bool set_target(const unsigned TPindex, const GeoPoint &loc,
     const bool override_lock);

  /**
   * Set target location from a range and radial
   * referenced on the bearing from the previous target
   * used by dlgTarget
   *
   * @param range the range [0,1] from center to perimeter
   * of the oz
   *
   * @param radial the angle in degrees of the target
   */
  bool set_target(const unsigned TPindex, const fixed range,
     const fixed radial);

  /**
   * returns position of the target in range / radial format
   * referenced on the bearing from the previous target
   * used by dlgTarget
   *
   * @param &range returns the range [0,1] from center
   * to perimeter of the oz
   *
   * @param &radial returns the angle in degrees of
   * the target in the sector in polar coordinates
   */
  bool get_target_range_radial(const unsigned TPindex, fixed &range,
                               fixed &radial) const;

  /**
   * Lock/unlock the target from automatic shifts of specified tp
   *
   * @param TPindex index of tp in task
   * @param do_lock Whether to lock the target
   */
  bool target_lock(const unsigned TPindex, bool do_lock);

  /**
   * returns copy of name of specified ordered tp
   *
   * @param TPindex index of ordered tp in task
   */
  gcc_pure
  const TCHAR* get_ordered_taskpoint_name(const unsigned TPindex) const;

  /**
   * Accessor for location of specified ordered tp
   *
   * @param TPindex index of tp in ordered task
   *
   * @return location of tp or fallback_location if
   * TPindex is invalid
   */
  gcc_pure
  const GeoPoint& get_ordered_taskpoint_location(const unsigned TPindex,
     const GeoPoint& fallback_location) const;

  /**
   * Accessor for oz radius of specified ordered tp
   *
   * @param TPindex index of tp in ordered task
   *
   * @return oz radius or 0 if
   * TPindex is invalid
   */
  gcc_pure
  fixed get_ordered_taskpoint_radius(const unsigned TPindex) const;

  void route_set_terrain(const RasterTerrain *terrain);
  void route_solve(const AGeoPoint& dest, const AGeoPoint& start,
                   const short h_ceiling);
  void route_update_polar(const SpeedVector& wind);
  GlidePolar get_reach_polar() const;

  bool intersection(const AGeoPoint& origin,
                    const AGeoPoint& destination,
                    GeoPoint& intx) const;

  void solve_reach(const AGeoPoint& origin, const bool do_solve);

  void accept_in_range(const GeoBounds& bounds,
                       TriangleFanVisitor& visitor) const;

  bool find_positive_arrival(const AGeoPoint& dest,
                             short& arrival_height_reach,
                             short& arrival_height_direct) const;

  short get_terrain_base() const;
};

#endif
