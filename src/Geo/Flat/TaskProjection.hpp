// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlatProjection.hpp"
#include "Geo/GeoBounds.hpp"

struct GeoPoint;

/**
 * Class for performing Lambert Conformal Conic projections from
 * ellipsoidal Earth points to and from projected points.  Has
 * converters for projected coordinates in integer and double types.
 *
 * Needs to be initialized with reset() before first use.
 */
class TaskProjection : public FlatProjection {
  GeoBounds bounds;

public:
#ifndef NDEBUG
  TaskProjection():bounds(GeoBounds::Invalid()) {}
#else
  TaskProjection() = default;
#endif

  explicit TaskProjection(const GeoBounds &bounds);

  /**
   * Reset search bounds
   *
   * @param ref Default value for initial search bounds
   */
  void Reset(const GeoPoint &ref);

  /**
   * Check a location against bounds and update them if outside.
   * This does not update the projection itself.
   *
   * @param ref Point to check against bounds
   * @return true if the bounds have been modified
   */
  bool Scan(const GeoPoint &ref) {
    return bounds.Extend(ref);
  }

  /**
   * Update projection.
   *
   * @return True if projection changed
   */
  bool Update();

  const auto &GetBounds() const noexcept {
    return bounds;
  }

  /** 
   * Calculate radius of points used in task projection
   * 
   * note: this is an approximation that should only
   * be used for rendering purposes
   *
   * @return Radius (m) from center to edge
   */
  [[gnu::pure]]
  double ApproxRadius() const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const TaskProjection& task_projection);
#endif
};
