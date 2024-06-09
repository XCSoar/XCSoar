// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SearchPointVector.hpp"

class FlatProjection;
class OZBoundary;
struct GeoPoint;
struct AircraftState;

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
class SampledTaskPoint {
  /**
   * Whether boundaries are used in scoring distance, or just the
   * reference point
   */
  const bool boundary_scored;

  /**
   * True when the current task state is past this task point.  This
   * is used to determine whether the task point was missed.  In that
   * case, a 'cheat' has to be applied.
   */
  bool past;

  SearchPointVector nominal_points;
  SearchPointVector sampled_points;
  SearchPointVector boundary_points;
  SearchPoint search_max_total;
  SearchPoint search_max;
  SearchPoint search_min;

public:
  /**
   * Constructor.  Clears boundary and interior samples on
   * instantiation.  Must be followed by update_oz() after task
   * geometry is modified.
   *
   * @param location the reference location of this task point
   * @param is_scored Whether distance within OZ is scored
   *
   * @return Partially initialised object
   */
  SampledTaskPoint(const GeoPoint &location, const bool is_scored) noexcept;

  /** Reset the task (as if never flown) */
  void Reset() noexcept;

  const GeoPoint &GetLocation() const noexcept {
    return nominal_points.front().GetLocation();
  }

  /**
   * Accessor to retrieve location of the boundary polygon node
   * that produces the task's maximum distance.
   *
   * @return Location of max distance node
   */
  [[gnu::pure]]
  const GeoPoint &GetLocationMaxTotal() const noexcept {
    assert(search_max_total.IsValid());

    return search_max_total.GetLocation();
  };

  /**
   * Accessor to retrieve location of the sample/boundary polygon node
   * that produces the current maximum achievable task distance.
   *
   * @return Location of max distance node
   */
  [[gnu::pure]]
  const GeoPoint &GetLocationMax() const noexcept {
    assert(search_max.IsValid());

    return search_max.GetLocation();
  };

  /**
   * Accessor to retrieve location of the sample/boundary polygon
   * node that produces the current minimum achievable task distance.
   *
   * @return Location of minimum distance node
   */
  const GeoPoint &GetLocationMin() const noexcept {
    assert(search_min.IsValid());

    return search_min.GetLocation();
  };

  [[gnu::pure]]
  GeoPoint InterpolateLocationMinMax(double p) const noexcept {
    return GetLocationMin().Interpolate(GetLocationMax(), p);
  }

  /**
   * Construct boundary polygon from internal representation of observation zone.
   * Also updates projection.
   */
  void UpdateOZ(const FlatProjection &projection, const OZBoundary &boundary) noexcept;

protected:
  /**
   * Update the interior sample polygon.  The caller checks if the
   * given #AircraftState is inside the observation zone.
   *
   * @return True if internal state changed
   */
  bool AddInsideSample(const AircraftState &state,
                       const FlatProjection &projection) noexcept;

public:
  /**
   * Test if the task point has recorded presence of the aircraft
   * in this sector
   *
   * @return True if sample present
   */
  [[gnu::pure]]
  bool HasSampled() const noexcept {
    return !sampled_points.empty();
  }

  /**
   * Retrieve interior sample polygon (pure).
   *
   * @return Vector of sample points representing a closed polygon
   */
  [[gnu::pure]]
  const SearchPointVector &GetSampledPoints() const noexcept {
    return sampled_points;
  }

  /**
   * Retrieve boundary points polygon
   */
  const SearchPointVector &GetBoundaryPoints() const noexcept {
    assert(!boundary_points.empty());

    return boundary_points;
  }

  /**
   * Return a #SearchPointVector that contains just the reference
   * point.
   */
  const SearchPointVector &GetNominalPoints() const noexcept {
    return nominal_points;
  }

  bool IsBoundaryScored() const noexcept {
    return boundary_scored;
  }

protected:
  void SetPast(bool _past) noexcept {
    past = _past;
  }

  /**
   * Clear all sample points and add the current state as a sample.
   * This is used, for exmaple, for StartPoints to only remember the
   * last sample prior to crossing the start.
   */
  void ClearSampleAllButLast(const AircraftState &state,
                             const FlatProjection &projection) noexcept;

private:
  /**
   * Re-project boundary and interior sample polygons.
   * Must be called if task_projection changes.
   */
  void UpdateProjection(const FlatProjection &projection) noexcept;

public:
  /**
   * Retrieve interior sample polygon.
   *
   * Because sometimes an OZ will be skipped (by accident, true miss,
   * or failure of electronics), but we still want rest of task to
   * function, the 'cheat' option allows non-achieved task points to
   * be considered achieved by assuming the aircraft appeared at the
   * reference location.
   *
   * @return a list of boundary points
   */
  [[gnu::pure]]
  const SearchPointVector &GetSearchPoints() const noexcept;

  /**
   * Set the location of the boundary polygon node
   * that produces the task's maximum distance.
   *
   * @param locmax Location of max distance node
   */
  void SetSearchMaxTotal(const SearchPoint &locmax) noexcept {
    assert(locmax.IsValid());

    search_max_total = locmax;
  }
  
  /**
   * Set the location of the sample/boundary polygon node
   * that produces the current maximum achievable task distance.
   *
   * @param locmax Location of max distance node
   */
  void SetSearchMax(const SearchPoint &locmax) noexcept {
    assert(locmax.IsValid());

    search_max = locmax;
  }

  /**
   * Set the location of the sample/boundary polygon node
   * that produces the current minimum achievable task distance.
   *
   * @param locmin Location of min distance node
   */
  void SetSearchMin(const SearchPoint &locmin) noexcept {
    assert(locmin.IsValid());

    search_min = locmin;
  }
};
