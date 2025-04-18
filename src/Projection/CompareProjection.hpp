// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/Quadrilateral.hpp"

class WindowProjection;

/**
 * This class remembers the screen bounds of an existing Projection
 * object, and compares it after a change.  It is used to check if
 * calculation results from the previous frame are still valid, or if
 * they should be discarded.
 */
class CompareProjection {
  struct FourCorners : GeoQuadrilateral {
    FourCorners() = default;
    FourCorners(const WindowProjection &projection) noexcept;
  };

  FourCorners corners;

  double latitude_cos;

  double max_delta = -1;

public:
  /**
   * Creates a "cleared" object, so that comparisons are always false.
   */
  CompareProjection() noexcept = default;

  explicit CompareProjection(const WindowProjection &projection) noexcept;

  /**
   * Clears the object, so that comparisons are always false.  Useful
   * to Invalidate a cache.
   */
  void Clear() noexcept {
    max_delta = -1;
  }

  bool IsDefined() const noexcept {
    return max_delta > 0;
  }

  bool Compare(const CompareProjection &other) const noexcept;

  /**
   * Is the new projection close enough to the saved one?
   */
  bool Compare(const WindowProjection &projection) const noexcept {
    return Compare(CompareProjection(projection));
  }

  bool CompareAndUpdate(const CompareProjection &other) noexcept;

  /**
   * Is the new projection close enough to the saved one?  If not,
   * then the saved one is updated.
   */
  bool CompareAndUpdate(const WindowProjection &projection) noexcept {
    return CompareAndUpdate(CompareProjection(projection));
  }
};
