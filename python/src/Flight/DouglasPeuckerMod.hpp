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

#ifndef PYTHON_DOUGLASPEUCKERMOD_HPP
#define PYTHON_DOUGLASPEUCKERMOD_HPP

#include <vector>
#include <list>
#include <memory>
#include <queue>
#include <limits>

struct IGCFixEnhanced;
struct GeoPoint;

class DouglasPeuckerMod {
private:
  const unsigned num_levels;
  const unsigned zoom_factor;
  const double threshold;
  const bool force_endpoints;
  const unsigned max_delta_time;
  const unsigned max_points;

  double *zoom_level_breaks;


  class CompareDist {
  public:
    bool operator()(std::pair<unsigned, double> n1,
                    std::pair<unsigned ,double> n2) {
      if (n1.second < n2.second)
        return true;
      else
        return false;
    }
  };

  typedef std::priority_queue<std::pair<unsigned, double>,
                      std::vector<std::pair<unsigned, double>>,
                      CompareDist> DistQueue;

public:
  DouglasPeuckerMod(const unsigned _num_levels = 18,
                    const unsigned _zoom_factor = 2,
                    const double _threshold = 0.00001,
                    const bool _force_endpoints = true,
                    const unsigned _max_delta_time = 60,
                    const unsigned _max_points = std::numeric_limits<unsigned>::max());

  ~DouglasPeuckerMod();

  /**
   * Encode a list of fixes from start to end using Douglas-Peucker's algorithm.
   * Modifies the fixes array in-place (the 'level' attribute only).
   */
  void Encode(std::vector<IGCFixEnhanced> &fixes,
                const unsigned start, const unsigned end);

  unsigned GetNumLevels() {
    return num_levels;
  }

  unsigned GetZoomFactor() {
    return zoom_factor;
  }

private:
  /**
   * Calculate the perpendicular distance of a point to the
   * track line of two other points.
   */
  double DistanceGeo(const GeoPoint &p0,
                      const GeoPoint &p1,
                      const GeoPoint &p2);

  /**
   * Calculate a DouglasPeucker-like weight using the temporal
   * distance of a fix to it's adjacent fixes.
   */
  double DistanceTime(const unsigned time0,
                       const unsigned time1,
                       const unsigned time2);

  /**
   * This computes the appropriate zoom level of a point in terms of it's
   * distance from the relevant segment in the DP algorithm. Could be done in
   * terms of a logarithm, but this approach makes it a bit easier to ensure
   * that the level is not too large.
   */
  unsigned ComputeLevel(const double abs_max_dist);

  /**
   * Classify the level of each fix in fixes using the distance perpendicular
   * to the track line of the adjacent fixes. This modifies the fixes vector
   * in place from start to end.
   */
  void Classify(std::vector<IGCFixEnhanced> &fixes,
                DistQueue &dists,
                const double abs_max_dist,
                const unsigned start,
                const unsigned end);
};

#endif /* PYTHON_DOUGLASPEUCKERMOD_HPP */
