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

#ifndef SAMPLEDTASKPOINT_H
#define SAMPLEDTASKPOINT_H

#include "Navigation/SearchPointVector.hpp"
#include "ObservationZone.hpp"
#include "TaskWaypoint.hpp"
#include "Compiler.h"

class TaskEvents;
class TaskProjection;
struct GeoPoint;

/**
 * Abstract specialisation of TaskPoint which has an observation zone
 * and can manage records of the appearance of the aircraft within the
 * observation zone, as well as provide methods to scan for potential
 * paths to border locations.
 *
 * \todo
 * - Currently undefined as to what happens to interior samples if observation 
 *   zone is modified (e.g. due to previous/next taskpoint moving) in update_oz
 */
class SampledTaskPoint
  : public TaskWaypoint, public virtual ObservationZone
{
  friend class OrderedTask;
  friend class PrintHelper;

protected:
  const bool m_boundary_scored; /**< Whether boundaries are used in scoring distance, or just the reference point */

private:
  SearchPointVector m_nominal_point;
  SearchPointVector m_sampled_points;
  SearchPointVector m_boundary_points;
  SearchPoint m_search_max;
  SearchPoint m_search_min;
  SearchPoint m_search_reference;

public:
  /**
   * Constructor.  Clears boundary and interior samples on instantiation.
   * Must be followed by update_oz() after task geometry is modified.
   *
   * @param wp Waypoint associated with this task point
   * @param tb Task Behaviour defining options (esp safety heights)
   * @param b_scored Whether distance within OZ is scored
   *
   * @return Partially initialised object
   */
  SampledTaskPoint(enum type _type,
                   const Waypoint & wp,
                   const TaskBehaviour &tb,
                   const bool b_scored);

  virtual ~SampledTaskPoint() {};

  /**
   * Reset the task (as if never flown)
   *
   */
  virtual void reset();

  /**
   * Accessor to retrieve location of the sample/boundary polygon
   * node that produces the maximum task distance.
   *
   * @return Location of max distance node
   */
  gcc_pure
  const GeoPoint& get_location_max() const {
    return m_search_max.get_location();
  };

  /**
   * Accessor to retrieve location of the sample/boundary polygon
   * node that produces the minimum task distance.
   *
   * @return Location of minimum distance node
   */
  const GeoPoint& get_location_min() const {
    return m_search_min.get_location();
  };

  /**
   * Construct boundary polygon from internal representation of observation zone.
   * Also updates projection.
   */
  virtual void update_oz(const TaskProjection &projection);

  /**
   * Check if aircraft is within observation zone if near, and if so,
   * update the interior sample polygon.
   *
   * @param state Aircraft state
   * @param task_events Callback class for feedback
   *
   * @return True if internal state changed
   */
  virtual bool update_sample_near(const AIRCRAFT_STATE& state,
                                  TaskEvents &task_events,
                                  const TaskProjection &projection);

  /**
   * Perform updates to samples as required if known to be far from the OZ
   *
   * @param state Aircraft state
   * @param task_events Callback class for feedback
   *
   * @return True if internal state changed
   */
  virtual bool update_sample_far(const AIRCRAFT_STATE& state,
                                  TaskEvents &task_events,
                                 const TaskProjection &projection) { return false; }

  /**
   * Test if the task point has recorded presence of the aircraft
   * in this sector
   *
   * @return True if sample present
   */
  gcc_pure
  bool has_sampled() const {
    return !m_sampled_points.empty();
  }

  /**
   * Retrieve interior sample polygon (pure).
   *
   * @return Vector of sample points representing a closed polygon
   */
  gcc_pure
  const SearchPointVector& get_sample_points() const {
    return m_sampled_points;
  }

  bool is_boundary_scored() const {
    return m_boundary_scored;
  }

protected:

  /**
   * Clear all sample points and add the current state as a sample.
   * This is used, for exmaple, for StartPoints to only remember the last sample
   * prior to crossing the start.
   */
  void clear_sample_all_but_last(const AIRCRAFT_STATE& state,
                                 const TaskProjection &projection);

  /**
   * Set minimum distance point based on location.
   *
   * @param location Location of min point
   */
  void set_search_min(const GeoPoint &location,
                      const TaskProjection &projection);

  /**
   * Retrieve boundary points polygon
   */
  const SearchPointVector& get_boundary_points() const {
    return m_boundary_points;
  }

private:

  /**
   * Re-project boundary and interior sample polygons.
   * Must be called if task_projection changes.
   *
   */
  void update_projection(const TaskProjection &projection);

  /**
   * Determines whether to 'cheat' a missed OZ prior to the current active task point.
   *
   * @return Vector of boundary points representing a closed polygon
   */
  gcc_pure
  virtual bool search_nominal_if_unsampled() const = 0;

  /**
   * Determines whether to return sampled or boundary points for max/min search
   *
   * @return Vector of boundary points representing a closed polygon
   */
  gcc_pure
  virtual bool search_boundary_points() const = 0;

  /**
   * Retrieve interior sample polygon.
   * Because sometimes an OZ will be skipped (by accident, true miss, or
   * failure of electronics), but we still want rest of task to function,
   * the 'cheat' option allows non-achieved task points to be considered achieved
   * by assuming the aircraft appeared at the reference location.
   *
   * @return Vector of boundary points representing a closed polygon
   */
  gcc_pure
  const SearchPointVector& get_search_points() const;

  /**
   * Set the location of the sample/boundary polygon node
   * that produces the maximum task distance.
   *
   * @param locmax Location of max distance node
   */
  void set_search_max(const SearchPoint &locmax) {
    m_search_max = locmax;
  }

  /**
   * Set the location of the sample/boundary polygon node
   * that produces the minimum task distance.
   *
   * @param locmin Location of min distance node
   */
  void set_search_min(const SearchPoint &locmin) {
    m_search_min = locmin;
  }

  /**
   * Clear all sample points.
   *
   */
  void clear_sample_points();
};

#endif //SAMPLEDOBSERVATIONZONE_H
