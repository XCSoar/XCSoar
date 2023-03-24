// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AATIsoline.hpp"

/**
 *  Specialisation of AATIsoline such that the segment of
 *  the isoline within the task point's observation zone is
 *  determined.  This allows for parametric representation
 *  of all points along the isoline within the OZ.
 * 
 *  End-points of segments are searched for and so this
 *  class is slow to instantiate.
 */
class AATIsolineSegment: public AATIsoline
{
  double t_up;
  double t_down;

public:
  /**
   * Constructor.  This performs the search for the isoline
   * segment and so is slow.
   *
   * @param ap The AAT point for which the isoline is sought
   *
   * @return Initialised object
   */
  AATIsolineSegment(const AATPoint &ap,
                    const FlatProjection &projection) noexcept;

  /**
   * Test whether segment is valid (nonzero length)
   *
   * @return True if segment is valid
   */
  [[gnu::pure]]
  bool IsValid() const noexcept;

  /**
   * Parametric representation of points on the isoline segment.
   *
   * @param t Parameter (0,1)
   *
   * @return Location of point on isoline segment
   */
  [[gnu::pure]]
  GeoPoint Parametric(double t) const noexcept;
};
