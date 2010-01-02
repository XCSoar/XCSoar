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

#ifndef ORDEREDTASK_H
#define ORDEREDTASK_H

#include "AbstractTask.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include <vector>
#include "Util/Serialisable.hpp"
#include "GlideSolvers/MacCready.hpp"

class OrderedTaskPoint;

/**
 * A task comprising an ordered sequence of task points, each with
 * observation zones.  A valid OrderedTask has a StartPoint, zero or more
 * IntermediatePoints and a FinishPoint.
 *
 * \todo
 * - better handling of removal of start/finish point
 * - allow for TakeOffPoint and LandingPoint
 * - have a method to check if a potential taskpoint is distinct from its neighbours?
 * - multiple start points
 */
class OrderedTask:
  public AbstractTask,
  public Serialisable
{
public:
  friend class TaskDijkstra;

  typedef std::vector<OrderedTaskPoint*> OrderedTaskPointVector; /**< Storage type of task points */ 

  /** 
   * Constructor.
   *
   * \todo
   * - default values in constructor
   * 
   * @param te Task events
   * @param tb Task behaviour
   * @param ta Task advance
   * @param gp Glide Polar
   * 
   * @return Initialised object
   */
  OrderedTask(const TaskEvents &te, 
              const TaskBehaviour &tb,
              TaskAdvance &ta,
              GlidePolar &gp);
  ~OrderedTask();

  /** 
   * Reset the task (as if never flown)
   * 
   */
  void reset();

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Active task point
 */
  TaskPoint* getActiveTaskPoint() const;

/** 
 * Retrieves the active task point index.
 * 
 * @return Index of active task point sequence
 */
  unsigned getActiveIndex() const;

/** 
 * Retrieve task point by sequence index
 * 
 * @param index Index of task point sequence
 * 
 * @return OrderedTaskPoint at index (or NULL if out of range)
 */
  const OrderedTaskPoint* getTaskPoint(const unsigned index) const;

/**
 * Determine whether active task point optionally shifted points to
 * a valid task point.
 *
 * @param index_offset offset (default 0)
 */
  bool validTaskPoint(const int index_offset=0) const;

/** 
 * Check if task has a single StartPoint
 * 
 * @return True if task has start
 */
  bool has_start() const;

/** 
 * Check if task has a single FinishPoint
 * 
 * @return True if task has finish
 */
  bool has_finish() const;

/** 
 * Set active task point index
 * 
 * @param desired Desired active index of task sequence
 */
  void setActiveTaskPoint(unsigned desired);

/** 
 * Insert taskpoint before specified index in task.  May fail if the candidate
 * is the wrong type (e.g. if it is a StartPoint and the task already
 * has one).  
 * Ownership is transferred to this object.
 * 
 * @param tp Taskpoint to insert
 * @param position Index in task sequence, before which to insert
 * 
 * @return True on success
 */
  bool insert(OrderedTaskPoint* tp, const unsigned position);

/** 
 * Replace taskpoint.
 * May fail if the candidate is the wrong type.
 * Does nothing (but returns true) if replacement is equivalent
 * Ownership is transferred to this object.
 * 
 * @param tp Taskpoint to become replacement
 * @param position Index in task sequence of task point to replace
 * 
 * @return True on success
 */
  bool replace(OrderedTaskPoint* tp, const unsigned position);

/** 
 * Append taskpoint to end of task.  May fail if the candidate
 * is the wrong type (e.g. if it is a StartPoint and the task already
 * has one).
 * Ownership is transferred to this object.
 * 
 * @param tp Taskpoint to append to task
 * 
 * @return True on success
 */
  bool append(OrderedTaskPoint* tp);

/** 
 * Remove task point at specified position.  Note that
 * currently start/finish points can't be removed.
 * 
 * @param position Index in task sequence of task point to remove
 * 
 * @return True on success
 */
  bool remove(const unsigned position);

/** 
 * Check if task is valid.  Calls task_event methods on failure.
 * 
 * @return True if task is valid
 */
  bool check_task() const;

/** 
 * Test if task has finished.  Used to determine whether
 * or not to continue updating stats.
 * 
 * @return True if task is finished
 */
  bool task_finished() const;

/** 
 * Test if task has started.  Used to determine whether
 * or not update stats.
 * 
 * @return True if task has started
 */
  bool task_started() const;

/** 
 * Update internal states when aircraft state advances.
 * 
 * @param state_now Aircraft state at this time step
 * @param full_update Force update due to task state change
 *
 * @return True if internal state changes
 */
  bool update_sample(const AIRCRAFT_STATE &state_now, 
                     const bool full_update);


/** 
 * Update internal states (non-essential) for housework, or where functions are slow
 * and would cause loss to real-time performance.
 * 
 * @param state_now Aircraft state at this time step
 * 
 * @return True if internal state changed
 */
  bool update_idle(const AIRCRAFT_STATE& state_now);

/** 
 * Return size of task
 * 
 * @return Number of task points in task
 */
  unsigned task_size() const;

/** 
 * Accessor for task projection, for use when creating task points
 * 
 * @return Task global projection
 */
  TaskProjection& get_task_projection();

protected:

/** 
 * Retrieve vector of search points to be used in max/min distance
 * scans (by TaskDijkstra).
 * 
 * @param tp Index of task point of query
 * 
 * @return Vector of search point candidates
 */
  const SearchPointVector& get_tp_search_points(unsigned tp) const;

/** 
 * Set task point's minimum distance value (by TaskDijkstra).
 * 
 * @param tp Index of task point to set min
 * @param sol Search point found to be minimum distance
 */
  void set_tp_search_min(unsigned tp, const SearchPoint &sol);

/** 
 * Set task point's maximum distance value (by TaskDijkstra).
 * 
 * @param tp Index of task point to set max
 * @param sol Search point found to be maximum distance
 */
  void set_tp_search_max(unsigned tp, const SearchPoint &sol);

/** 
 * Scan task for valid start/finish points
 * 
 * @return True if start and finish found
 */
  bool scan_start_finish();

/** 
 * Test whether (and how) transitioning into/out of task points should occur, typically
 * according to task_advance mechanism.  This also may call the task_event callbacks.
 * 
 * @param state_now Aircraft state at this time step
 * @param state_last Aircraft state at previous time step
 * 
 * @return True if transition occurred
 */
  bool check_transitions(const AIRCRAFT_STATE &state_now, 
                         const AIRCRAFT_STATE &state_last);

/** 
 * Calculate distance of nominal task (sum of distances from each
 * leg's consecutive reference point to reference point for entire task).
 * 
 * @return Distance (m) of nominal task
 */ 
  fixed scan_distance_nominal();

/** 
 * Calculate distance of planned task (sum of distances from each leg's
 * achieved/scored reference points respectively for prior task points,
 * and targets or reference points for active and later task points).
 * 
 * @return Distance (m) of planned task
 */
  fixed scan_distance_planned();

/** 
 * Calculate distance of planned task (sum of distances from aircraft to
 * current target/reference and for later task points from each leg's
 * targets or reference points).
 * 
 * @param ref Location of aircraft
 * 
 * @return Distance (m) remaining in the planned task
 */ 
  fixed scan_distance_remaining(const GEOPOINT &ref);

/** 
 * Calculate scored distance of achieved part of task.
 * 
 * @param ref Location of aircraft
 * 
 * @return Distance (m) achieved adjusted for scoring
 */
  fixed scan_distance_scored(const GEOPOINT &ref);

/** 
 * Calculate distance of achieved part of task.
 * For previous taskpoints, the sum of distances of maximum distance
 * points; for current, the distance from previous max distance point to
 * the aircraft.
 * 
 * @param ref Location of aircraft
 * 
 * @return Distance (m) achieved
 */
  fixed scan_distance_travelled(const GEOPOINT &ref);

/** 
 * Calculate maximum and minimum distances for task, achievable
 * from the current aircraft state (assuming active taskpoint does not retreat). 
 * 
 * @param ref Aircraft location
 * @param full Perform full search (if task state has changed)
 * @param dmin Minimum distance (m) achievable of task
 * @param dmax Maximum distance (m) achievable of task
 */
  void scan_distance_minmax(const GEOPOINT &ref, 
                            bool full,
                            fixed *dmin, fixed *dmax);

/** 
 * Calculate task start time.
 * 
 * @param state_now Aircraft state
 * 
 * @return Time (s) of start of task
 */
  fixed scan_total_start_time(const AIRCRAFT_STATE &state_now);

/** 
 * Calculate leg start time.
 * 
 * @param state_now Aircraft state
 * 
 * @return Time (s) of start of leg
 */
  fixed scan_leg_start_time(const AIRCRAFT_STATE &state_now);


/** 
 * Calculate glide result for remainder of task
 * 
 * @param state_now Aircraft state
 * @param polar Glide polar used for calculation
 * @param total Glide result accumulated for total remaining task
 * @param leg Glide result for current leg of task
 */
  void glide_solution_remaining(const AIRCRAFT_STATE &state_now, 
                                const GlidePolar &polar,
                                GlideResult &total,
                                GlideResult &leg);

/** 
 * Calculate glide result from start of task to current state
 * 
 * @param state_now Aircraft state
 * @param total Glide result accumulated for total travelled task
 * @param leg Glide result for current leg of task
 */
  void glide_solution_travelled(const AIRCRAFT_STATE &state_now, 
                                GlideResult &total,
                                GlideResult &leg);

/** 
 * Calculate glide result from start of task to finish, and from this
 * calculate the effective position of the aircraft along the task based
 * on the remaining time.  This system therefore allows effective speeds
 * to be calculated which take into account the time value of height.
 * 
 * @param state_now Aircraft state
 * @param total Glide result accumulated for total task
 * @param leg Glide result for current leg of task
 * @param total_remaining_effective 
 * @param leg_remaining_effective 
 * @param total_t_elapsed Total planned task time (s)
 * @param leg_t_elapsed Leg planned task time (s)
 */
  void glide_solution_planned(const AIRCRAFT_STATE &state_now, 
                              GlideResult &total,
                              GlideResult &leg,
                              DistanceRemainingStat &total_remaining_effective,
                              DistanceRemainingStat &leg_remaining_effective,
                              const fixed total_t_elapsed,
                              const fixed leg_t_elapsed);

/** 
 * Calculate/search for best MC, being the highest MC value to produce a
 * pure glide solution for the remainder of the task.
 * 
 * @param state_now Aircraft state
 * 
 * @return Best MC value found (m/s)
 */
  fixed calc_mc_best(const AIRCRAFT_STATE &state_now);

/** 
 * Calculate virtual sink rate of aircraft that allows a pure glide solution
 * for the remainder of the task.  Glide is performed according to Mc theory
 * speed with the current glide polar, neglecting effect of virtual sink rate.
 * 
 * @param state_now Aircraft state
 * 
 * @return Sink rate of aircraft (m/s)
 */
  fixed calc_glide_required(const AIRCRAFT_STATE &state_now);

/** 
 * Calculate cruise efficiency for the travelled part of the task.
 * This is the ratio of the achieved inter-thermal cruise speed to that
 * predicted by MacCready theory with the current glide polar.
 * 
 * @param state_now Aircraft state
 * 
 * @return Cruise efficiency (0-1)
 */
  fixed calc_cruise_efficiency(const AIRCRAFT_STATE &state_now);

  fixed calc_effective_mc(const AIRCRAFT_STATE &state_now);

/** 
 * Optimise target ranges (for adjustable tasks) to produce an estimated
 * time remaining with the current glide polar equal to a target value.
 * This adjusts the target locations for the remainder of the task.
 * 
 * @param state_now Aircraft state
 * @param t_target Desired time for remainder of task (s)
 * 
 * @return Target range parameter (0-1)
 */
  fixed calc_min_target(const AIRCRAFT_STATE &state_now, 
                        const fixed t_target);

/** 
 * Calculate angle from aircraft to remainder of task (minimum of leg
 * heights above turnpoints divided by distance to go for each leg).  
 * 
 * @param state_now Aircraft state
 * 
 * @return Minimum gradient angle of remainder of task
 */
  fixed calc_gradient(const AIRCRAFT_STATE &state_now);

private:

  fixed scan_distance_min(const GEOPOINT &ref, bool full);
  fixed scan_distance_max();

/** 
 * Sets previous/next taskpoint pointers for task point at specified
 * index in sequence.
 * 
 * @param position Index of task point
 */
  void set_neighbours(unsigned position);

/** 
 * Checks whether a new task point is suitable to add to the
 * task based on type of task point and whether task already
 * has a start/finish point.
 * 
 * @param new_tp Candidate task point to add
 * 
 * @return True if ok to add to task
 */
  bool check_startfinish(OrderedTaskPoint* new_tp); 

/** 
 * Update internal geometric state of task points. 
 * Typically called after task geometry or observation zones are modified.
 * 
 *
 * This also updates planned/nominal distances so clients can use that
 * data during task construction.
 */
  void update_geometry();

/** 
 * Erase taskpoint in sequence (for internal use)
 * 
 * @param i index of task point in sequence
 */
  void erase(unsigned i);

  OrderedTaskPointVector tps;

  StartPoint *ts;

  FinishPoint *tf;

  TaskProjection task_projection;

  bool distance_is_significant(const GEOPOINT &location,
                               const GEOPOINT &location_last) const;

  GEOPOINT m_location_min_last;

public:

#ifdef DO_PRINT
  void print(const AIRCRAFT_STATE &state);
#endif

/** 
 * Accept a task point visitor; makes the visitor visit
 * all TaskPoint in the task
 * 
 * @param visitor Visitor to accept
 * @param reverse Visit task points in reverse order 
 *
 * \todo reverse not implemented yet
 */
  void Accept(TaskPointVisitor& visitor, const bool reverse=false) const;

  DEFINE_VISITABLE()
};

#endif //ORDEREDTASK_H
