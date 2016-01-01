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

#ifndef SAMPLEDTASKPOINT_H
#define SAMPLEDTASKPOINT_H

#include "Geo/SearchPointVector.hpp"
#include "Compiler.h"

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
  SampledTaskPoint(const GeoPoint &location, const bool is_scored);

  /** Reset the task (as if never flown) */
  void Reset();

  const GeoPoint &GetLocation() const {
    return nominal_points.front().GetLocation();
  }

  /**
   * Accessor to retrieve location of the sample/boundary polygon node
   * that produces the maximum task distance.
   *
   * @return Location of max distance node
   */
  gcc_pure
  const GeoPoint &GetLocationMax() const {
    assert(search_max.IsValid());

    return search_max.GetLocation();
  };

  /**
   * Accessor to retrieve location of the sample/boundary polygon
   * node that produces the minimum task distance.
   *
   * @return Location of minimum distance node
   */
  const GeoPoint &GetLocationMin() const {
    assert(search_min.IsValid());

    return search_min.GetLocation();
  };

  gcc_pure
  GeoPoint InterpolateLocationMinMax(double p) const {
    return GetLocationMin().Interpolate(GetLocationMax(), p);
  }

  /**
   * Construct boundary polygon from internal representation of observation zone.
   * Also updates projection.
   */
  void UpdateOZ(const FlatProjection &projection, const OZBoundary &boundary);

protected:
  /**
   * Update the interior sample polygon.  The caller checks if the
   * given #AircraftState is inside the observation zone.
   *
   * @return True if internal state changed
   */
  bool AddInsideSample(const AircraftState &state,
                       const FlatProjection &projection);

public:
  /**
   * Test if the task point has recorded presence of the aircraft
   * in this sector
   *
   * @return True if sample present
   */
  gcc_pure
  bool HasSampled() const {
    return !sampled_points.empty();
  }

  /**
   * Retrieve interior sample polygon (pure).
   *
   * @return Vector of sample points representing a closed polygon
   */
  gcc_pure
  const SearchPointVector &GetSampledPoints() const {
    return sampled_points;
  }

  /**
   * Retrieve boundary points polygon
   */
  const SearchPointVector &GetBoundaryPoints() const {
    assert(!boundary_points.empty());

    return boundary_points;
  }

  /**
   * Return a #SearchPointVector that contains just the reference
   * point.
   */
  const SearchPointVector &GetNominalPoints() const {
    return nominal_points;
  }

  bool IsBoundaryScored() const {
    return boundary_scored;
  }

protected:
  void SetPast(bool _past) {
    past = _past;
  }

  /**
   * Clear all sample points and add the current state as a sample.
   * This is used, for exmaple, for StartPoints to only remember the
   * last sample prior to crossing the start.
   */
  void ClearSampleAllButLast(const AircraftState &state,
                             const FlatProjection &projection);

private:
  /**
   * Re-project boundary and interior sample polygons.
   * Must be called if task_projection changes.
   */
  void UpdateProjection(const FlatProjection &projection);

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
  gcc_pure
  const SearchPointVector &GetSearchPoints() const;

  /**
   * Set the location of the sample/boundary polygon node
   * that produces the maximum task distance.
   *
   * @param locmax Location of max distance node
   */
  void SetSearchMax(const SearchPoint &locmax) {
    assert(locmax.IsValid());

    search_max = locmax;
  }

  /**
   * Set the location of the sample/boundary polygon node
   * that produces the minimum task distance.
   *
   * @param locmin Location of min distance node
   */
  void SetSearchMin(const SearchPoint &locmin) {
    assert(locmin.IsValid());

    search_min = locmin;
  }
};

#endif //SAMPLEDOBSERVATIONZONE_H
