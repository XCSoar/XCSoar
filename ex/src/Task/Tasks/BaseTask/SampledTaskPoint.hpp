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

#ifndef SAMPLEDTASKPOINT_H
#define SAMPLEDTASKPOINT_H

#include "Navigation/SearchPointVector.hpp"
#include "ObservationZone.hpp"
#include "TaskPoint.hpp"
#include "Navigation/TaskProjection.hpp"

class TaskEvents;

/**
 * Abstract specialisation of TaskPoint which has an observation zone
 * and can manage records of the appearance of the aircraft within the
 * observation zone, as well as provide methods to scan for potential
 * paths to border locations.
 *
 * \todo
 * - Currently undefined as to what happens to interior samples if observation 
 *   zone is modified (e.g. due to previous/next taskpoint moving)
 */
class SampledTaskPoint:
  public TaskPoint, 
  public virtual ObservationZone
{
public:  
  friend class OrderedTask;

/** 
 * Constructor.  Clears boundary and interior samples on instantiation.
 * Must be followed by update_oz() after task geometry is modified.
 * 
 * @param tp Projection used for internal representations of borders and samples
 * @param wp Waypoint associated with this task point
 * @param tb Task Behaviour defining options (esp safety heights)
 * @param b_scored Whether distance within OZ is scored 
 * 
 * @return Partially initialised object 
 */
  SampledTaskPoint(const TaskProjection& tp,
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
  const GEOPOINT& get_location_max() const {
    return m_search_max.get_location();
  };

/** 
 * Accessor to retrieve location of the sample/boundary polygon
 * node that produces the minimum task distance.
 * 
 * @return Location of minimum distance node
 */
  const GEOPOINT& get_location_min() const {
    return m_search_min.get_location();
  };

/** 
 * Construct boundary polygon from internal representation of observation zone.
 * Also updates projection.
 */
  virtual void update_oz();

/** 
 * Check if aircraft is within observation zone, and if so,
 * update the interior sample polygon.
 * 
 * @param state Aircraft state
 * @param task_events Callback class for feedback
 *
 * @return True if internal state changed
 */
  virtual bool update_sample(const AIRCRAFT_STATE& state,
                             const TaskEvents &task_events);

/** 
 * Accessor for task projection
 * 
 * @return Task projection used by this point
 */
  const TaskProjection &get_task_projection() const {
    return m_task_projection;
  };

/** 
 * Retrieve interior sample polygon (pure).
 * 
 * @return Vector of sample points representing a closed polygon
 */
  const SearchPointVector& get_sample_points() const;

protected:

/** 
 * Clear all sample points and add the current state as a sample.
 * This is used, for exmaple, for StartPoints to only remember the last sample
 * prior to crossing the start.
 */  
  void clear_sample_all_but_last(const AIRCRAFT_STATE& state);

  const bool m_boundary_scored; /**< Whether boundaries are used in scoring distance, or just the reference point */

private:

/** 
 * Re-project boundary and interior sample polygons.
 * Must be called if task_projection changes.
 * 
 */
  void update_projection();

/** 
 * Determines whether to 'cheat' a missed OZ prior to the current active task point. 
 *
 * @return Vector of boundary points representing a closed polygon
 */
  virtual bool search_nominal_if_unsampled() = 0;

/** 
 * Determines whether to return sampled or boundary points for max/min search
 *
 * @return Vector of boundary points representing a closed polygon
 */
  virtual bool search_boundary_points() = 0;

/** 
 * Retrieve interior sample polygon.
 * Because sometimes an OZ will be skipped (by accident, true miss, or
 * failure of electronics), but we still want rest of task to function,
 * the 'cheat' option allows non-achieved task points to be considered achieved
 * by assuming the aircraft appeared at the reference location.
 * 
 * @return Vector of boundary points representing a closed polygon
 */
    const SearchPointVector& get_search_points();

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

  const TaskProjection &m_task_projection;

/** 
 * Clear all sample points.
 * 
 */
  void clear_sample_points();

  SearchPointVector m_sampled_points;
  SearchPointVector m_boundary_points;
  SearchPoint m_search_max;
  SearchPoint m_search_min;
  SearchPoint m_search_reference;

public:
#ifdef DO_PRINT
  virtual void print(std::ostream& f, const AIRCRAFT_STATE&state) const;
  virtual void print_samples(std::ostream& f, const AIRCRAFT_STATE&state);
#endif

};
#endif //SAMPLEDOBSERVATIONZONE_H
