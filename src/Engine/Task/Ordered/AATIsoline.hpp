// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoEllipse.hpp"

class AATPoint;

/**
 * Object representing an isoline, being the locus of potential target
 * points within an AATPoint's observation zone such that all points
 * have constant double-leg distance (distance from previous max to
 * this point to next planned).
 *
 * Internally, this is represented by an ellipse in flat-earth
 * projection.
 */
class AATIsoline
{
protected:
  /** ellipse representing the isoline segment */
  const GeoEllipse ell;

public:
  /**
   * Constructor.
   *
   * @param ap The AAT point for which to calculate the Isoline
   */
  AATIsoline(const AATPoint &ap, const FlatProjection &projection) noexcept;
};
