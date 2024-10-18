// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoVector.hpp"
#include "Geo/Memento/DistanceMemento.hpp"
#include "Geo/Memento/GeoVectorMemento.hpp"

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

  DistanceMemento memo_max_total;
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
  TaskLeg(OrderedTaskPoint &_destination) noexcept
    :destination(_destination) {}

  /**
   * Calculate distance of nominal task (sum of distances from each
   * leg's consecutive reference point to reference point for entire task).
   *
   * @return Distance (m) of nominal task
   */
  [[gnu::pure]]
  double ScanDistanceNominal() const noexcept;

  /**
   * Calculate distance of planned task (sum of distances from each leg's
   * achieved/scored reference points respectively for prior task points,
   * and targets or reference points for active and later task points).
   *
   * @return Distance (m) of planned task
   */
  [[gnu::pure]]
  double ScanDistancePlanned() noexcept;

  /**
   * Calculate maximum task distance, irrespective of path flown
   * (sum of distances from legs between all maximum distance points)
   *
   * @return Distance (m) of task's maximum distance
   */
  [[gnu::pure]]
  double ScanDistanceMaxTotal() const noexcept;

  /**
   * Calculate distance of maximum achievable task (sum of distances from
   * each leg's achieved/scored points respectively for prior task points,
   * and maximum distance points for active and later task points).
   *
   * @return Distance (m) of maximum achievable task
   */
  [[gnu::pure]]
  double ScanDistanceMax() const noexcept;

  /**
   * Calculate distance of minimum achievable task (sum of distances from
   * each leg's achieved/scored points respectively for prior task points,
   * and minimum distance points for active and later task points).
   *
   * @return Distance (m) of minimum achievable task
   */
  double ScanDistanceMin() const noexcept;

  /**
   * Calculate distance of planned task (sum of distances from aircraft to
   * current target/reference and for later task points from each leg's
   * targets or reference points).
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) remaining in the planned task
   */
  double ScanDistanceRemaining(const GeoPoint &ref) noexcept;

  /**
   * Calculate scored distance of achieved part of task.
   *
   * @param ref Location of aircraft
   *
   * @return Distance (m) achieved adjusted for scoring
   */
  double ScanDistanceScored(const GeoPoint &ref) const noexcept;

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
  double ScanDistanceTravelled(const GeoPoint &ref) noexcept;

  /**
   * Retrieve maximum distance for the task leg
   *
   * @return Distance (m)
   */
  [[gnu::pure]]
  double GetMaximumTotalLegDistance() const noexcept;
  
  /**
   * Retrieve maximum possible leg distance
   *
   * @return Distance (m)
   */
  [[gnu::pure]]
  double GetMaximumLegDistance() const noexcept;

  /**
   * Retrieve min possible leg distance
   *
   * @return Distance (m)
   */
  [[gnu::pure]]
  double GetMinimumLegDistance() const noexcept;

  /**
   * Retrieve nominal leg distance
   *
   * @return Distance (m)
   */
  [[gnu::pure]]
  double GetNominalLegDistance() const noexcept {
    return GetNominalLegVector().distance;
  }

  [[gnu::pure]]
  GeoVector GetNominalLegVector() const noexcept;

  /**
   * Calculate vector from aircraft to destination
   */
  const GeoVector &GetVectorRemaining() const noexcept {
    return vector_remaining;
  }

  /**
   * Calculate vector from aircraft to destination
   */
  const GeoVector &GetVectorPlanned() const noexcept {
    return vector_planned;
  }

  /**
   * Calculate vector travelled along this leg
   */
  const GeoVector &GetVectorTravelled() const noexcept {
    return vector_travelled;
  }

private:
  [[gnu::pure]]
  GeoVector GetPlannedVector() const noexcept;
  
  [[gnu::pure]]
  GeoVector GetTravelledVector(const GeoPoint &ref) const noexcept;
  
  [[gnu::pure]]
  GeoVector GetRemainingVector(const GeoPoint &ref) const noexcept;

  [[gnu::pure]]
  double GetScoredDistance(const GeoPoint &ref) const noexcept;

  [[gnu::pure]]
  const OrderedTaskPoint *GetOrigin() const noexcept;

  [[gnu::pure]]
  const OrderedTaskPoint *GetNext() const noexcept;

  [[gnu::pure]]
  OrderedTaskPoint *GetNext() noexcept;
};
