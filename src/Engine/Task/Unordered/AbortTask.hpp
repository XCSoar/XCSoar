/* Copyright_License {

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

#ifndef ABORTTASK_H
#define ABORTTASK_H

#include "UnorderedTask.hpp"
#include "UnorderedTaskPoint.hpp"

#include <vector>

#include <assert.h>

class Waypoints;
class AbortIntersectionTest;
class AlternateList;

/**
 * Abort task provides automatic management of a sorted list of task points
 * that are reachable or close to reachable, and landable (with airfields preferred).
 *
 * @todo
 *  - should prefer landable points that are non-intersecting with terrain
 *
 * Sorting order is as follows:
 * - airfields reachable from final glide (sorted by arrival time) at safety mc
 * - landpoints reachable from final glide (sorted by arrival time) at safety mc
 * - airfields reachable with climb (sorted by arrival time including climb time) 
 *   at current mc
 * - landpoints reachable with climb (sorted by arrival time including climb time) 
 *   at current mc
 * 
 */
class AbortTask: public UnorderedTask
{
  /** max number of items in list */
  static constexpr unsigned max_abort = 10;

protected:
  struct AlternateTaskPoint {
    UnorderedTaskPoint point;
    GlideResult solution;

    AlternateTaskPoint(WaypointPtr &&waypoint, const TaskBehaviour &tb,
                       const GlideResult &_solution)
      :point(std::move(waypoint), tb), solution(_solution) {}
  };

  typedef std::vector<AlternateTaskPoint> AlternateTaskVector;
  AlternateTaskVector task_points;

  /** whether the AbortTask is the master or running in background */
  bool is_active;

  const Waypoints &waypoints;

  /** Hook for external intersection tests */
  AbortIntersectionTest* intersection_test;

private:
  unsigned active_waypoint;
  bool reachable_landable;

public:
  /** 
   * Base constructor.
   * 
   * @param tb Global task behaviour settings
   * @param gp Global glide polar used for navigation calculations
   * @param wps Waypoints container to be scanned during updates
   * 
   * @return Initialised object (with nothing in task)
   */
  AbortTask(const TaskBehaviour &tb,
            const Waypoints &wps);

  void SetTaskBehaviour(const TaskBehaviour &tb);

  const UnorderedTaskPoint &GetAlternate(unsigned i) const {
    assert(i < task_points.size());

    return task_points[i].point;
  }

  /**
   * Retrieves the active task point index.
   *
   * @return Index of active task point sequence
   */
  unsigned GetActiveIndex() const {
    return active_task_point;
  }

  /**
   * Determine if any landable reachable waypoints were found in the
   * last update.
   *
   * @return True if a landable waypoint was found
   */
  bool HasReachableLandable() const {
    return reachable_landable;
  }

  /**
   * Calculate vector to home waypoint
   *
   * @param state State of aircraft
   * @return Vector to home waypoint
   */
  GeoVector GetHomeVector(const AircraftState &state) const;
  WaypointPtr GetHome() const;

protected:
  /**
   * Clears task points in list
   */
  virtual void Clear();

  /**
   * Check whether abort task list is full
   *
   * @return True if no more task points can be added
   */
  gcc_pure
  bool IsTaskFull() const;

  /**
   * Calculate distance to search for landable waypoints for aircraft.
   *
   * @param state_now Aircraft state
   *
   * @return Distance (m) of approximate glide range of aircraft
   */
  gcc_pure
  double GetAbortRange(const AircraftState &state_now,
                       const GlidePolar &glide_polar) const;

  /**
   * Fill abort task list with candidate waypoints given a list of
   * waypoints satisfying approximate range queries.  Can be used
   * to add airfields only, or landpoints.
   *
   * @param state Aircraft state
   * @param approx_waypoints List of candidate waypoints
   * @param polar Polar used for tests
   * @param only_airfield If true, only add waypoints that are airfields.
   * @param final_glide Whether solution must be glide only or climb allowed
   * @param safety Whether solution uses safety polar
   *
   * @return True if a landpoint within final glide was found
   */
  bool FillReachable(const AircraftState &state,
                     AlternateList &approx_waypoints,
                     const GlidePolar &polar, bool only_airfield,
                     bool final_glide, bool safety);

protected:
  /**
   * This is called by update_sample after the turnpoint list has 
   * been filled with landpoints.
   * It's first called after the reachable scan, then may be called again after scanning
   * for unreachable.
   */
  virtual void ClientUpdate(const AircraftState &state_now, bool reachable);

public:
  /**
   * Specify whether the task is active or not.  If it's active, it will
   * notify the user about the abort task point being changed via the events.
   */
  void SetActive(bool _active) {
    is_active = _active;
  }

  /**
   * Set external test function to be used for additional intersection tests
   */
  void SetIntersectionTest(AbortIntersectionTest *test) {
    intersection_test = test;
  }

  /**
   * Accept a const task point visitor; makes the visitor visit
   * all TaskPoint in the task
   *
   * @param visitor Visitor to accept
   * @param reverse Visit task points in reverse order
   */
  void AcceptTaskPointVisitor(TaskPointConstVisitor &visitor) const override;

public:
  /* virtual methods from class TaskInterface */
  virtual unsigned TaskSize() const override;
  virtual void SetActiveTaskPoint(unsigned index) override;
  virtual TaskWaypoint *GetActiveTaskPoint() const override;
  virtual bool IsValidTaskPoint(int index_offset) const override;

  /* virtual methods from class AbstractTask */
  virtual void Reset() override;

protected:
  virtual bool UpdateSample(const AircraftState &state_now,
                            const GlidePolar &glide_polar,
                            bool full_update) override;
};

#endif
