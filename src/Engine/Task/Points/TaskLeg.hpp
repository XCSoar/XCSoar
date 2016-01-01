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

#ifndef TASKLEG_H
#define TASKLEG_H

#include "Geo/GeoVector.hpp"
#include "Geo/Memento/DistanceMemento.hpp"
#include "Geo/Memento/GeoVectorMemento.hpp"
#include "Compiler.h"

struct AircraftState;

class OrderedTaskPoint;

/**
 * Utility class for use by OrderedTaskPoint to provide methods
 * for calculating task segments (and accumulations thereof) according
 * to various metrics.
 *  
 * All of the scan_ methods propagate forwards to the end of the task.
 * Some of these, e.g. ScanDistanceRemaining() can be called from
 * the current task point, whereas others (e.g. ScanDistanceNominal)
 * should be called from the StartPoint.
 *
 * This class uses mementos to reduce expensive re-calculation of static data.
 */
class TaskLeg {
  /** Saved vector for current leg's travelled route */
  GeoVector vector_travelled;
  /** Saved vector for current leg's remaining route */
  GeoVector vector_remaining;
  /** Saved vector for current leg's planned route */
  GeoVector vector_planned;

  DistanceMemento memo_max;
  DistanceMemento memo_min;
  GeoVectorMemento memo_nominal;
  GeoVectorMemento memo_planned;
  GeoVectorMemento memo_travelled;
  GeoVectorMemento memo_remaining;

  OrderedTaskPoint& destination;

public:
  /**
   * Constructor.  Takes local copy of taskpoint data used in internal computations
   */
  TaskLeg(OrderedTaskPoint &_destination)
    :destination(_destination) {}

  /**
   * Calculate distance of nominal task (sum of distances from each
   * leg's consecutive reference point to reference point for entire task).
   *
   * @return Distance (m) of nominal task
   */
  gcc_pure
  double ScanDistanceNominal() const;

  /**
   * Calculate distance of planned task (sum of distances from each leg's
   * achieved/scored reference points respectively for prior task points,
   * and targets or reference points for active and later task points).
   *
   * @return Distance (m) of planned task
   */
  gcc_pure
  double ScanDistancePlanned();

  /**
   * Calculate distance of maximum achievable task (sum of distances from
   * each leg's achieved/scored points respectively for prior task points,
   * and maximum distance points for active and later task points).
   *
   * @return Distance (m) of maximum achievable task
   */
  gcc_pure
  double ScanDistanceMax() const;

  /**
   * Calculate distance of minimum achievable task (sum of distances from
   * each leg's achieved/scored points respectively for prior task points,
   * and minimum distance points for active and later task points).
   *
   * @return Distance (m) of minimum achievable task
   */
  double ScanDistanceMin() const;

  /**
   * Calculate distance of planned task (sum of distances from aircraft to
   * current target/reference and for later task points from each leg's
   * targets or reference points).
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) remaining in the planned task
   */
  double ScanDistanceRemaining(const GeoPoint &ref);

  /**
   * Calculate scored distance of achieved part of task.
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) achieved adjusted for scoring
   */
  double ScanDistanceScored(const GeoPoint &ref) const;

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
  double ScanDistanceTravelled(const GeoPoint &ref);

  /**
   * Retrieve maximum possible leg distance
   *
   * @return Distance (m)
   */
  gcc_pure
  double GetMaximumLegDistance() const;
  
  /**
   * Retrieve min possible leg distance
   *
   * @return Distance (m)
   */
  gcc_pure
  double GetMinimumLegDistance() const;

  /**
   * Retrieve nominal leg distance
   *
   * @return Distance (m)
   */
  gcc_pure
  double GetNominalLegDistance() const {
    return GetNominalLegVector().distance;
  }

  gcc_pure
  GeoVector GetNominalLegVector() const;

  /**
   * Calculate vector from aircraft to destination
   */
  const GeoVector &GetVectorRemaining() const {
    return vector_remaining;
  }

  /**
   * Calculate vector from aircraft to destination
   */
  const GeoVector &GetVectorPlanned() const {
    return vector_planned;
  }

  /**
   * Calculate vector travelled along this leg
   */
  const GeoVector &GetVectorTravelled() const {
    return vector_travelled;
  }

private:
  gcc_pure
  GeoVector GetPlannedVector() const;
  
  gcc_pure
  GeoVector GetTravelledVector(const GeoPoint &ref) const;
  
  gcc_pure
  GeoVector GetRemainingVector(const GeoPoint &ref) const;

  gcc_pure
  double GetScoredDistance(const GeoPoint &ref) const;

  gcc_pure
  const OrderedTaskPoint *GetOrigin() const;

  gcc_pure
  const OrderedTaskPoint *GetNext() const;

  gcc_pure
  OrderedTaskPoint *GetNext();
};

#endif
