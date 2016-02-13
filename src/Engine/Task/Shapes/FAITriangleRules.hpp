/*
Copyright_License {

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

#ifndef XCSOAR_FAI_TRIANGLE_RULES_HPP
#define XCSOAR_FAI_TRIANGLE_RULES_HPP

#include "Compiler.h"

struct GeoPoint;
struct FAITriangleSettings;

namespace FAITriangleRules
{
  /**
   * The minimum leg percentage for "small FAI triangles".
   */
  static constexpr double SMALL_MIN_LEG(0.28);

  /**
   * The maximum leg percentage for "small FAI triangles".  This is a
   * derived value, assuming the other two legs are as short as
   * possible.
   */
  static constexpr double SMALL_MAX_LEG(1 - 2 * SMALL_MIN_LEG);

  /**
   * The threshold which allows applying the "large FAI triangle"
   * rules [m].
   *
   * Don't use this variable.  Use FAITriangleSettings::GetThreshold()
   * instead.
   *
   * @see FAITriangleSettings::Threshold::FAI
   */
  static constexpr double LARGE_THRESHOLD_FAI(750000);

  /**
   * Relaxed threshold used by some contests such as OLC and DMSt.
   *
   * Don't use this variable.  Use FAITriangleSettings::GetThreshold()
   * instead.
   *
   * @see FAITriangleSettings::Threshold::KM500
   */
  static constexpr double LARGE_THRESHOLD_500(500000);

  /**
   * The minimum leg percentage for "large FAI triangles".
   */
  static constexpr double LARGE_MIN_LEG(0.25);

  /**
   * The maximum leg percentage for "large FAI triangles".
   */
  static constexpr double LARGE_MAX_LEG(0.45);

  static constexpr inline double LargeMinLeg(double total) {
    return total / 4.;
  }

  gcc_pure
  bool TestDistances(double d1, double d2, double d3,
                     const FAITriangleSettings &settings);

  gcc_pure
  bool TestDistances(const GeoPoint &a, const GeoPoint &b, const GeoPoint &c,
                     const FAITriangleSettings &settings);
}

#endif
