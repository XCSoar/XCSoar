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

#ifndef OLC_TRIANGLE_RULES_HPP
#define OLC_TRIANGLE_RULES_HPP

#include "Engine/Task/Shapes/FAITriangleRules.hpp"

#include <algorithm>

struct GeoPoint;
class FlatProjection;

struct OLCTriangleConstants {
  static inline constexpr double large_threshold_m = 500000; // 500km for DMSt/OLC
};

class OLCTriangleValidator : OLCTriangleConstants {
  unsigned large_threshold_flat;

public:
  explicit OLCTriangleValidator(unsigned _large_threshold_flat) noexcept
    :large_threshold_flat(_large_threshold_flat) {}

  /**
   * Calculates if the given candidate set is feasible
   * (i.e. it might contain a feasible triangle).
   * Use relaxed checks to ensure distance errors due to the flat projection
   * or integer rounding don't invalidate close positives.
   */
  [[gnu::pure]]
  bool IsFeasible(unsigned df_min, unsigned df_max,
                  unsigned shortest_max, unsigned longest_min) const noexcept {
    // shortest leg min 28% (here: 27.5%) for small triangle,
    // min 25% (here: 24.3%) for large triangle
    if ((df_max > large_threshold_flat && shortest_max * 37 < df_min * 9) ||
        (df_max <= large_threshold_flat && shortest_max * 40 < df_min * 11)) {
      return false;
    }

    // longest leg max 45% (here: 47%)
    if (longest_min * 19 > df_max * 9)
      return false;

    return true;
  }

  template<typename P, typename GetMaxDistance, typename GetLocation>
  [[gnu::pure]]
  bool IsIntegral(const P &tp1, const P &tp2, const P &tp3,
                  unsigned shortest_max, unsigned longest_max,
                  GetMaxDistance &&get_max_distance,
                  GetLocation &&get_location) const noexcept {
    // Solution is integral, calculate rough distance for fast checks
    const unsigned df_total = get_max_distance(tp1, tp2) +
      get_max_distance(tp2, tp3) +
      get_max_distance(tp3, tp1);

    // fast checks, as in IsFeasible

    // shortest >= 28.2% * df_total
    if (shortest_max * 39 >= df_total * 11)
      return true;

    // longest >= 45.8% * df_total
    if (longest_max * 24 > df_total * 11)
      return false;

    // small triangle and shortest < 27.5% df_total
    if (df_total < large_threshold_flat && shortest_max * 40 < df_total * 11)
      return false;

    // detailed checks
    const auto geo_tp1 = get_location(tp1);
    const auto geo_tp2 = get_location(tp2);
    const auto geo_tp3 = get_location(tp3);

    const unsigned d_12 = unsigned(geo_tp1.Distance(geo_tp2));
    const unsigned d_23 = unsigned(geo_tp2.Distance(geo_tp3));
    const unsigned d_31 = unsigned(geo_tp3.Distance(geo_tp1));

    const unsigned d_total = d_12 + d_23 + d_31;

    return d_total >= unsigned(large_threshold_m)
      ? FAITriangleRules::CheckLargeTriangle(d_12, d_23, d_31)
      : FAITriangleRules::CheckSmallTriangle(d_12, d_23, d_31);
  }
};

struct OLCTriangleRules : private OLCTriangleConstants {
  [[gnu::pure]]
  static OLCTriangleValidator MakeValidator(const FlatProjection &projection,
                                            const GeoPoint &reference) noexcept;
};

#endif
