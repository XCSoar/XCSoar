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

#ifndef TASKLEG_H
#define TASKLEG_H

#include "Navigation/Geometry/GeoVector.hpp"
#include "Navigation/Memento/DistanceMemento.hpp"
#include "Navigation/Memento/GeoVectorMemento.hpp"
#include "Compiler.h"

struct AIRCRAFT_STATE;

class OrderedTaskPoint;

/**
 * Utility class for use by OrderedTaskPoint to provide methods
 * for calculating task segments (and accumulations thereof) according
 * to various metrics.
 *  
 * All of the scan_ methods propagate forwards to the end of the task.
 * Some of these, e.g. scan_distance_remaining() can be called from
 * the current task point, whereas others (e.g. scan_distance_nominal)
 * should be called from the StartPoint.
 *
 * This class uses mementos to reduce expensive re-calculation of static data.
 */
class TaskLeg {

protected:
  /** Saved vector for current leg's travelled route */
  GeoVector vector_travelled;
  /** Saved vector for current leg's remaining route */
  GeoVector vector_remaining;
  /** Saved vector for current leg's planned route */
  GeoVector vector_planned;

private:
  DistanceMemento memo_max;
  DistanceMemento memo_min;
  DistanceMemento memo_nominal;
  GeoVectorMemento memo_planned;
  GeoVectorMemento memo_travelled;
  GeoVectorMemento memo_remaining;

  OrderedTaskPoint& destination;

public:
  /**
   * Constructor.  Takes local copy of taskpoint data used in internal computations
   */
  TaskLeg(OrderedTaskPoint &_destination);

  /**
   * Calculate distance of nominal task (sum of distances from each
   * leg's consecutive reference point to reference point for entire task).
   *
   * @return Distance (m) of nominal task
   */
  fixed scan_distance_nominal() const;

  /**
   * Calculate distance of planned task (sum of distances from each leg's
   * achieved/scored reference points respectively for prior task points,
   * and targets or reference points for active and later task points).
   *
   * @return Distance (m) of planned task
   */
  fixed scan_distance_planned();

  /**
   * Calculate distance of maximum achievable task (sum of distances from
   * each leg's achieved/scored points respectively for prior task points,
   * and maximum distance points for active and later task points).
   *
   * @return Distance (m) of maximum achievable task
   */
  fixed scan_distance_max() const;

  /**
   * Calculate distance of minimum achievable task (sum of distances from
   * each leg's achieved/scored points respectively for prior task points,
   * and minimum distance points for active and later task points).
   *
   * @return Distance (m) of minimum achievable task
   */
  fixed scan_distance_min() const;

  /**
   * Calculate distance of planned task (sum of distances from aircraft to
   * current target/reference and for later task points from each leg's
   * targets or reference points).
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) remaining in the planned task
   */
  fixed scan_distance_remaining(const GeoPoint &ref);

  /**
   * Calculate scored distance of achieved part of task.
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) achieved adjusted for scoring
   */
  fixed scan_distance_scored(const GeoPoint &ref) const;

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
  fixed scan_distance_travelled(const GeoPoint &ref);

  /**
   * Retrieve maximum possible leg distance
   *
   * @return Distance (m)
   */
  gcc_pure
  fixed leg_distance_max() const;
  
  /**
   * Retrieve min possible leg distance
   *
   * @return Distance (m)
   */
  gcc_pure
  fixed leg_distance_min() const;

  /**
   * Retrieve nominal leg distance
   *
   * @return Distance (m)
   */
  gcc_pure
  fixed leg_distance_nominal() const;

private:
  gcc_pure
  GeoVector leg_vector_planned() const;
  
  gcc_pure
  GeoVector leg_vector_travelled(const GeoPoint &ref) const;
  
  gcc_pure
  GeoVector leg_vector_remaining(const GeoPoint &ref) const;

  gcc_pure
  fixed leg_distance_scored(const GeoPoint &ref) const;

  gcc_pure
  const OrderedTaskPoint* origin() const;

  gcc_pure
  OrderedTaskPoint* next() const;
};

#endif
