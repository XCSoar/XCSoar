// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/ZeroFinder.hpp"

class GeoEllipse;
class AATPoint;

/**
 *  Calculate where Isoline ellipse crosses border of observation zone
 */
class IsolineCrossingFinder final : public ZeroFinder
{
  const AATPoint &aap;
  const GeoEllipse &ell;

public:
  /**
   * Constructor.  After construction, call solve() to perform the search.
   *
   * @param _aap AATPoint for which to test OZ inclusion
   * @param _ell GeoEllipse representing the isoline
   * @param xmin Min parameter of search
   * @param xmax Max parameter of search
   *
   * @return Partially initialised object
   */
  IsolineCrossingFinder(const AATPoint& _aap, const GeoEllipse &_ell,
                        const double xmin, const double xmax) noexcept;

  double f(const double t) noexcept;

  /**
   * Test validity of solution
   * @param t parametric location of test point
   * @return True if valid
   */
  bool valid(const double t) noexcept;

  /**
   * Search for parameter value of isoline intersecting the OZ boundary
   * within min/max parameter range.
   *
   * @return Parameter value of isoline intersection
   */
  double solve() noexcept;
};
