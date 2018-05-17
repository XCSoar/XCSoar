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

  Original code of Douglas-Peucker algorithm by Robert Coup <robert.coup@koordinates.com>
*/

#include "DouglasPeuckerMod.hpp"
#include "IGCFixEnhanced.hpp"
#include "Geo/GeoPoint.hpp"

#include <stack>
#include <vector>
#include <list>
#include <utility>
#include <algorithm>

DouglasPeuckerMod::DouglasPeuckerMod(const unsigned _num_levels,
                                     const unsigned _zoom_factor,
                                     const double _threshold,
                                     const bool _force_endpoints,
                                     const unsigned _max_delta_time,
                                     const unsigned _max_points)
  : num_levels(_num_levels), zoom_factor(_zoom_factor),
    threshold(_threshold), force_endpoints(_force_endpoints),
    max_delta_time(_max_delta_time), max_points(_max_points) {
  zoom_level_breaks = new double[num_levels];

  for (unsigned i = 0; i < num_levels; i++) {
    zoom_level_breaks[i] = threshold * pow(zoom_factor, num_levels - i - 1);
  }
}

DouglasPeuckerMod::~DouglasPeuckerMod() {
  delete[] zoom_level_breaks;
}

void DouglasPeuckerMod::Encode(std::vector<IGCFixEnhanced> &fixes,
                                             const unsigned start, const unsigned end) {
  unsigned max_loc = 0;
  std::stack<std::pair<unsigned, unsigned>> stack;
  const unsigned fixes_size = end - start;
  DistQueue dists;

  double temp,
         max_dist,
         abs_max_dist_squared = 0.0,
         abs_max_dist,
         threshold_squared = pow(threshold, 2);

  /**
   * use normal douglas peucker distance (perpendicular to segment) for lon/lat
   * and use simple distance calculation for time
   */

  // simplify using Douglas-Peucker
  if (fixes_size > 2) {
    stack.push(std::pair<unsigned, unsigned>(start, end - 1));

    while (stack.size() > 0) {
      std::pair<unsigned, unsigned> current = stack.top();
      stack.pop();
      max_dist = 0;

      for (unsigned i = current.first + 1; i < current.second; i++) {
        temp = std::max(DistanceGeo(fixes[i].location,
                                    fixes[current.first].location,
                                    fixes[current.second].location),
                        DistanceTime(fixes[i].clock,
                                     fixes[current.first].clock,
                                     fixes[current.second].clock));

        if (temp > max_dist) {
          max_dist = temp;
          max_loc = i;
        }
      }

      if (max_dist > abs_max_dist_squared) {
        abs_max_dist_squared = max_dist;
      }

      if (max_dist > threshold_squared) {
        dists.push(std::pair<unsigned, double>(max_loc, sqrt(max_dist)));
        stack.push(std::pair<unsigned, unsigned>(current.first, max_loc));
        stack.push(std::pair<unsigned, unsigned>(max_loc, current.second));
      }
    }
  }

  abs_max_dist = sqrt(abs_max_dist_squared);

  Classify(fixes, dists, abs_max_dist, start, end);
}

/**
 * distance(p0, p1, p2) computes the distance between the point p0 and the
 * segment [p1,p2]. This could probably be replaced with something that is a
 * bit more numerically stable.
 */
double DouglasPeuckerMod::DistanceGeo(const GeoPoint &p0,
                                       const GeoPoint &p1,
                                       const GeoPoint &p2) {
  double u,
         out = 0.0,
         u_nom = 0.0,
         u_denom = 0.0;

  if (p1.longitude == p2.longitude &&
      p1.latitude == p2.latitude) {
    out += pow(p2.longitude.Degrees() - p0.longitude.Degrees(), 2);
    out += pow(p2.latitude.Degrees() - p0.latitude.Degrees(), 2);
  } else {
    u_nom += (p0.longitude.Degrees() - p1.longitude.Degrees())
             * (p2.longitude.Degrees() - p1.longitude.Degrees());
    u_nom += (p0.latitude.Degrees() - p1.latitude.Degrees())
             * (p2.latitude.Degrees() - p1.latitude.Degrees());

    u_denom += pow(p2.longitude.Degrees() - p1.longitude.Degrees(), 2);
    u_denom += pow(p2.latitude.Degrees() - p1.latitude.Degrees(), 2);

    u = u_nom / u_denom;

    if (u <= 0) {
      out += pow(p0.longitude.Degrees() - p1.longitude.Degrees(), 2);
      out += pow(p0.latitude.Degrees() - p1.latitude.Degrees(), 2);
    } else if (u >= 1) {
      out += pow(p0.longitude.Degrees() - p2.longitude.Degrees(), 2);
      out += pow(p0.latitude.Degrees() - p2.latitude.Degrees(), 2);
    } else if (0 < u && u < 1) {
      out += pow(p0.longitude.Degrees() - p1.longitude.Degrees()
             - u * (p2.longitude.Degrees() - p1.longitude.Degrees()), 2);
      out += pow(p0.latitude.Degrees() - p1.latitude.Degrees()
             - u * (p2.latitude.Degrees() - p1.latitude.Degrees()), 2);
    }
  }

  return out;
}

double DouglasPeuckerMod::DistanceTime(const unsigned time0,
                                        const unsigned time1,
                                        const unsigned time2) {
  double out = sqrt(abs(int(time1 - time0)) / max_delta_time * threshold) +
               sqrt(abs(int(time2 - time0)) / max_delta_time * threshold);

  out = pow(out, 2)/4;

  return pow(out, 2);
}

void DouglasPeuckerMod::Classify(std::vector<IGCFixEnhanced> &fixes,
                                 DistQueue &dists,
                                 const double abs_max_dist,
                                 const unsigned start,
                                 const unsigned end) {

  /* initialize levels with -1 */
  for (unsigned i = 0; i < fixes.size(); ++i) {
    fixes[i].level = -1;
  }

  // return early if start == end
  if (start == end) {
    fixes[start].level = 0;
    return;
  }

  unsigned i = force_endpoints ? 2 : 0;
  while (!dists.empty() && ++i < max_points) {
    std::pair<unsigned, double> fix_dist = dists.top();
    fixes[fix_dist.first].level = ComputeLevel(fix_dist.second);
    dists.pop();
  }

  /* Level for start- and endpoint */
  if (force_endpoints) {
    fixes[start].level = 0;
    fixes[end - 1].level = 0;
  }
}

unsigned DouglasPeuckerMod::ComputeLevel(const double abs_max_dist) {
  unsigned lev = 0;

  if (abs_max_dist > threshold) {
    while (abs_max_dist < zoom_level_breaks[lev]) {
      lev++;
    }
  }

  return lev;
}
