/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "FAITriangleRules.hpp"
#include "FAITriangleSettings.hpp"
#include "Geo/Math.hpp"

bool
FAITriangleRules::TestDistances(const double d1, const double d2, const double d3,
                                const FAITriangleSettings &settings) noexcept
{
  const auto d_wp = d1 + d2 + d3;

  /*
   * A triangle is a valid FAI-triangle, if no side is less than
   * 28% of the total length (total length less than 750 km), or no
   * side is less than 25% or larger than 45% of the total length
   * (totallength >= 750km).
   */

  return d_wp >= settings.GetThreshold()
    ? CheckLargeTriangle(d1, d2, d3)
    : CheckSmallTriangle(d1, d2, d3);
}

bool
FAITriangleRules::TestDistances(const GeoPoint &a, const GeoPoint &b,
                                const GeoPoint &c,
                                const FAITriangleSettings &settings) noexcept
{
  return TestDistances(Distance(a, b), Distance(b, c), Distance(c, a),
                       settings);
}
