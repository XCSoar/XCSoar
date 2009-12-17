/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include <vector>
#include "Waypoint/Waypoints.hpp"

/**
 * Abort task provides automatic management of a sorted list of task points
 * that are reachable or close to reachable, and landable (with airfields preferred).
 */
class AbortTask : 
  public UnorderedTask 
{
public:

  typedef std::vector < Waypoint > WaypointVector; /**< Vector of waypoints used to store candidates */

  /** 
   * Base constructor.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param ta Advance mechanism used for advancable tasks
   * @param gp Global glide polar used for navigation calculations
   * @param wps Waypoints container to be scanned during updates
   * 
   * @return Initialised object (with nothing in task)
   */
  AbortTask(const TaskEvents &te, 
            const TaskBehaviour &tb,
            TaskAdvance &ta,
            GlidePolar &gp,
            const Waypoints &wps);
  ~AbortTask();

/** 
 * Size of task
 * 
 * @return Number of taskpoints in task
 */
  unsigned task_size() const;

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Index of active task point sequence
 */
  TaskPoint* getActiveTaskPoint() const;

/** 
 * Set active task point index
 * 
 * @param index Desired active index of task sequence
 */
  void setActiveTaskPoint(unsigned index);

/**
 * Determine whether active task point optionally shifted points to
 * a valid task point.
 *
 * @param index_offset offset (default 0)
 */
  bool validTaskPoint(const int index_offset=0) const;

/** 
 * Update internal states when aircraft state advances.
 * This performs a scan of reachable waypoints.
 * 
 * \todo
 * - check tracking of active waypoint
 *
 * @param state_now Aircraft state at this time step
 * @param full_update Force update due to task state change
 *
 * @return True if internal state changes
 */
  virtual bool update_sample(const AIRCRAFT_STATE &state_now, 
                             const bool full_update);

  void reset();

protected:

/** 
 * Test whether (and how) transitioning into/out of task points should occur, typically
 * according to task_advance mechanism.  This also may call the task_event callbacks.
 * 
 * @param state_now Aircraft state at this time step
 * @param state_last Aircraft state at previous time step
 * 
 * @return True if transition occurred
 */
  virtual bool check_transitions(const AIRCRAFT_STATE& state_now, 
                                 const AIRCRAFT_STATE& state_last);

/** 
 * Clears task points in list
 * 
 */
  void clear();

/** 
 * Check whether abort task list is full
 * 
 * @return True if no more task points can be added
 */
  bool task_full() const;

/** 
 * Calculate distance to search for landable waypoints for aircraft.
 * 
 * @param state_now Aircraft state
 * 
 * @return Distance (m) of approximate glide range of aircraft
 */
  double abort_range(const AIRCRAFT_STATE &state_now);

/** 
 * Propagate changes to safety glide polar from global glide polar. 
 * 
 */
  void update_polar();

/** 
 * Prune out non-landable (landable or airport flagged) waypoints from
 * candidates 
 *
 * @param approx_waypoints Waypoint list to prune
 */
  void remove_unlandable(WaypointVector &approx_waypoints);

/** 
 * Fill abort task list with candidate waypoints given a list of
 * waypoints satisfying approximate range queries.  Can be used
 * to add airfields only, or landpoints.
 *
 * @param state Aircraft state
 * @param approx_waypoints List of candidate waypoints
 * @param only_airfield If true, only add waypoints that are airfields.
 */
  void fill_reachable(const AIRCRAFT_STATE &state,
                      WaypointVector &approx_waypoints,
                      const bool only_airfield);

  typedef std::pair<Waypoint,double> WP_ALT; /**< Class used to hold sorting data, second item is arival altitude */

  /**
   * Function object used to rank waypoints by arrival altitude
   */
  struct Rank : public std::binary_function<WP_ALT, WP_ALT, bool> {
    /**
     * Condition, ranks by arrival altitude 
     */
    bool operator()(const WP_ALT& x, const WP_ALT& y) const {
      return x.second > y.second;
    }
  };

private:
  std::vector<TaskPoint*> tps;
  unsigned active_waypoint;
  const Waypoints &waypoints;
  GlidePolar polar_safety;

public:
#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE &location);
#endif

/** 
 * Accept a task point visitor; makes the visitor visit
 * all TaskPoint in the task
 * 
 * @param visitor Visitor to accept
 */
  void Accept(TaskPointVisitor& visitor, const bool reverse=false) const;
  DEFINE_VISITABLE()
};

#endif //ABORTTASK_H
